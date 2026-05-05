# AGENTS.md

Guide for AI agents working in this repository.

## Project Overview

ryu_ldn_nx is an Atmosphere (Switch CFW) sysmodule that bridges Nintendo's LDN (Local Data Network) multiplayer to Ryujinx's LDN servers over TCP. It ships as TitleID `4200000000000010` (boot2). Three IPC services register simultaneously:

1. **`ldn:u` MITM** — intercepts Nintendo's LDN user service. Implements `ICommunicationService` with a `CommState` state machine (None → Initialized → AccessPoint/Station → AccessPointCreated/StationConnected). Game-visible `NetworkInfo`, node IDs, and advertise data are synthesized from server responses.

2. **`bsd:u` MITM** — intercepts BSD socket calls. Sockets that `bind`/`connect` to the LDN subnet **10.114.x.x** are tracked as proxy sockets and their traffic is tunneled through `ProxyData` packets. This is how gameplay UDP/TCP traffic reaches Ryujinx servers without pcap.

3. **`ryu:cfg`** — custom non-MITM IPC service the Tesla overlay talks to for live configuration and status display.

All three services share a 384 KB expanded heap (`g_heap_memory`). The `new`/`delete` operators are overridden to route to this heap via `lmem::ExpHeap`. Do not grow buffers casually.

## Build Commands

All cross-compilation runs inside Docker (`devkitpro/devkita64`). Use docker-compose, not bare `make` on the host.

```bash
# Sysmodule (.nsp) — builds libstratosphere first automatically
docker-compose run --rm build

# Tesla overlay (.ovl)
docker-compose run --rm overlay

# Both, in parallel with file-lock coordination
docker-compose run --rm all

# Host unit tests (g++, not cross-compiled; uses -DTEST_BUILD)
docker-compose run --rm test

# Clean sysmodule artifacts only
docker-compose run --rm clean
```

### Per-suite test targets

```bash
cd tests && make test-ldn-state-machine       # see Makefile for full list
make COVERAGE=1 coverage                        # gcov report
```

Available suite names: `protocol`, `config`, `config-manager`, `log`, `socket`, `tcp-client`, `connection-state`, `reconnect`, `client`, `ldn-types`, `ldn-state-machine`, `ldn-proxy`, `ldn-error`, `ldn-integration`, `overlay`, `ipc-config`, `config-ipc-service`, `shared-state`, `packet-dispatcher`, `session-handler`, `proxy-handler`, `handler-integration`, `upnp`, `p2p-proxy`, `p2p-client`, `p2p-integration`, `p2p-create-network`.

### Distribution packaging

```bash
cd sysmodule && make dist   # produces ryu_ldn_nx-sysmodule.zip with /atmosphere/contents/4200000000000010/ layout
```

### Debugging

```bash
docker-compose run --rm debugger <SWITCH_IP> [PID]   # interactive GDB session
```

GDB presets and component-specific debug scripts live in `scripts/debugger/`.

## Architecture & Control Flow

```
Game (ldn:u IPC)                Game (bsd:u IPC)              Tesla overlay
      │                               │                            │
      ▼                               ▼                            ▼
 ICommunicationService          BsdMitmService              ConfigService (ryu:cfg)
      │                               │                            │
      ▼                               ▼                            │
 LdnStateMachine ──▶ LdnPacketDispatcher                    LdnSharedState
      │                      │                                   ▲
      ▼                      ▼                                   │
 LdnSessionHandler    LdnProxyHandler ◀── P2pProxyClient/P2pProxyServer
      │                      │
      ▼                      ▼
 RyuLdnClient (network/)  ProxySocket / ProxySocketManager
      │                      │
      ▼                      ▼
 TcpClient + ConnectionState + Reconnect        BSD socket calls
      │
      ▼
 Ryujinx LDN server (TCP)
```

### Key source directories

| Path | Role |
|------|------|
| `sysmodule/source/main.cpp` | Entry point, heap, `new`/`delete`, service registration, thread pools |
| `sysmodule/source/protocol/` | Wire format (`types.hpp`, `ryu_protocol.hpp`, `packet_buffer.hpp`). **Binary-compatible with C# server — do not modify padding** |
| `sysmodule/source/network/` | `TcpClient`, `Client` (high-level), `ConnectionStateMachine`, `ReconnectManager`, `Socket` wrapper, `dns_wrap.cpp` |
| `sysmodule/source/ldn/` | LDN MITM service, `ICommunicationService`, `LdnStateMachine`, `LdnPacketDispatcher`, `LdnSessionHandler`, `LdnProxyHandler`, `LdnSharedState`, `LdnNodeMapper`, `LdnNetworkTimeout` |
| `sysmodule/source/bsd/` | BSD MITM service, `ProxySocket`, `ProxySocketManager`, `EphemeralPortPool` |
| `sysmodule/source/p2p/` | P2P proxy client/server, UPnP port mapper (`-lminiupnpc`) |
| `sysmodule/source/config/` | Fixed-buffer INI parser, `ConfigManager`, `ConfigIpcService`, baked-in game whitelist (~40 KB) |
| `sysmodule/source/debug/` | File logger with 2 s idle-timeout close thread, circular buffer for overlay |
| `overlay/` | Tesla overlay (libultrahand), IPC stubs in `ryu_ldn_ipc.c` |
| `tests/` | Host unit tests, one `.cpp` per suite |

## Wire Format — Critical Details

The protocol in `sysmodule/source/protocol/types.hpp` must remain **byte-for-byte compatible** with the C# server (`LdnServer/Network/RyuLdnProtocol.cs`). The C# server uses `[StructLayout(LayoutKind.Sequential, Size=0xA)]` **without** `Pack=1`, which means:

- `LdnHeader` is **12 bytes** (not 10): `magic(4) + type(1) + version(1) + reserved(2) + data_size(4)`. The `reserved` field pads `data_size` onto a 4-byte boundary at offset 8.
- `ConnectRequest` has explicit `_padding` at offset 0x7C to align `NetworkInfo` at offset 0x80 (8-byte alignment for `int64_t local_communication_id` in `IntentId`).
- `ExternalProxyConnectionState` has 3 bytes of `_pad` after the `connected` field for Pack=4 alignment.
- Every struct uses `__attribute__((packed))` and has a `static_assert` for its expected size. **If you see a `static_assert` failure, the fix is in the struct layout, not in the assert.**

Reference sources for cross-checking wire format:
- `~/GIT/ryujinx/` — emulator C# client
- `~/GIT/ldn/` — Ryujinx LDN server
- `~/GIT/ldn_mitm/` — original Switch sysmodule

**Do not second-guess the padding.** It has been verified repeatedly against Ryujinx source. If a bug looks like a protocol-padding mismatch, investigate game behavior, proxy socket routing, or state machine transitions first.

## Memory Constraints

The sysmodule runs on Switch hardware with aggressive constraints:

- **Heap**: 384 KB expanded heap (`g_heap_memory` in `main.cpp:143`). Previously 96 KB, saturated under real gameplay traffic causing DABRT 0x101 on allocation failure.
- **Malloc buffer**: 1 MB (`MallocBufferSize`) for the TLS heap central. Minimum for `TlsHeapCentral` to initialize properly.
- **Thread stacks**: MITM threads use 32 KB (`0x8000`). Config thread: 8 KB (`0x2000`). Log thread: 4 KB (`0x1000`). miniupnpc's `upnpDiscover()` needs this headroom.
- **BSD sessions**: 14 (`ConcurrencyLimitMax` in libstratosphere). The default of 3 saturated with P2P loopback sessions.
- **Total sysmodule budget**: ~10 MB across all Switch sysmodules — don't grow buffers without proof it's needed.
- Use fixed buffers, `constinit` statics, and stack-allocated work areas. Avoid `std::vector`/`std::deque`/`new` in hot paths.

## Code Conventions

- **C++ style**: 4-space indentation, opening braces on same line, Doxygen `/** */` doc comments on public API.
- **Namespace**: `ams::mitm::ldn` for LDN service code, `ams::mitm::bsd` for BSD MITM, `ryu_ldn::network` for network client, `ryu_ldn::protocol` for protocol, `ryu_ldn::config` for config, `ryu_ldn::debug` for logging, `ryu_ldn::ipc` for config IPC service.
- **Result patterns**: Use `ams::Result` (Stratosphere Horizon result codes). Test code uses simple `enum class ...Result` with `Success/...Error`.
- **Logging**: Use `LOG_ERROR`, `LOG_WARN`, `LOG_INFO`, `LOG_VERBOSE` macros from `debug/log.hpp`. The global logger is `ryu_ldn::debug::g_logger`. On Switch, uses `os::SdkMutex`; on host tests, `std::mutex`.
- **Platform guards**: `#ifdef __SWITCH__` for Switch-specific code (e.g., `stratosphere.hpp`, `os::SdkMutex`). Test builds use `-DTEST_BUILD` and compile with host `g++`.
- **`#pragma once`** for all headers (no include guards pattern).

## Test Patterns

- Tests in `tests/` are host-compiled (`g++ -std=c++17 -Wall -Wextra -g -O0 -I../sysmodule/source -DTEST_BUILD`).
- Each suite has its own Makefile target: `make test-<name>` runs just that suite.
- Lightweight custom test framework: `TEST(name)` macro, `ASSERT_TRUE`/`ASSERT_EQ`/`ASSERT_NE` macros, auto-registration via `register_test()`.
- Some suites link implementation objects (e.g., `config.o`, `tcp_client.o`); others are standalone (e.g., `ldn_state_machine_tests.cpp` inlines everything).
- `constinit` and `static_assert` in `types.hpp` are verified at compile time in both test and sysmodule builds.

## Configuration

- INI file at `sdmc:/config/ryu_ldn_nx/config.ini` (template: `config/ryu_ldn_nx/config.ini.example`).
- Sections: `[server]` (host, port, use_tls), `[network]` (timeouts, reconnect), `[ldn]` (enabled, passphrase, disable_p2p), `[debug]` (enabled, level, log_to_file).
- All config strings use fixed-size buffers — no `std::string` or dynamic allocation.
- Default-on-failure semantics: if config is missing or malformed, defaults are used so the sysmodule still functions.
- Config is loaded once at startup in `InitializeSystemModule()`. Live changes go through `ryu:cfg` IPC.

## Config Service (ryu:cfg)

The Tesla overlay communicates with `ryu:cfg` via `overlay/source/ryu_ldn_ipc.c` IPC stubs. The config service (`sysmodule/source/config/config_ipc_service.cpp`) exposes:

- Real-time LDN status (state, connected peers, network info)
- Live configuration changes (server host/port, debug level)
- Connection management (reconnect, disconnect)

The `LdnSharedState` struct (`ldn_shared_state.cpp`) bridges the LDN MITM and config service so the overlay can display current state.

## P2P Module

The P2P subsystem (`sysmodule/source/p2p/`) provides an optional direct P2P path using UPnP port mapping (`-lminiupnpc`):

- `P2pProxyServer` — listens for incoming P2P connections from other peers
- `P2pProxyClient` — connects to peer proxy servers for data relay
- `UpnpPortMapper` — handles UPnP port forward discovery and mapping

DNS resolution for UPnP is handled via `dns_wrap.cpp` which wraps `getaddrinfo`/`freeaddrinfo`/`getnameinfo` to `inet_pton`-based stubs, because `sfdnsres` causes DABRT in the boot2 context. Linker flags `--wrap=getaddrinfo --wrap=freeaddrinfo --wrap=getnameinfo` redirect all calls (including from miniupnpc) to these stubs.

The `bsd:s` service type is used instead of `bsd:u` because UPnP's `upnpDiscover()` requires privileged socket options (`IP_MULTICAST_TTL`, `IP_ADD_MEMBERSHIP`) that `bsd:u` blocks with EPERM.

## Build System Details

- The sysmodule Makefile inherits from `Atmosphere-libs/config/templates/stratosphere.mk` and produces `.nsp`, `.nso`, `.npdm`, `.elf` outputs.
- `libstratosphere` is built as a dependency before the sysmodule (handled by `docker-compose` service `libstratosphere`).
- The overlay Makefile uses `libultrahand` (submodule in `overlay/libultrahand/`) and produces a `.ovl` Tesla overlay.
- Build wrapper (`scripts/builder/build-wrapper.sh`) handles file-lock coordination between parallel `docker-compose` services, with logs in `build-logs/`.
- CI: `.github/workflows/build.yml` runs tests → builds sysmodule → builds overlay → packages. `.github/workflows/release.yml` additionally downloads game whitelist from the LDN server repo and creates a GitHub release with changelog.

## DCO & Commits

All commits must be DCO-signed (`git commit -s`). The project enforces this via `CONTRIBUTING.md` and PR checks. Conventional commits format is preferred.

**NEVER add promotional or attribution lines to commit messages** (no "Generated with Crush", "Assisted-by: AI", "Co-authored-by: AI", or similar). Only the author's own `Signed-off-by:` trail.

## IDE Noise

The host IDE will show errors for `<stratosphere.hpp>`, libnx APIs, and Switch-specific headers because devkitPro only exists in the Docker container. This is expected and intentional. Do not spend time trying to resolve these "errors" — they compile fine in the container.

## Debugging on Hardware

- Log file on Switch: `config/ryu_ldn_nx/ryu_ldn_nx.log` (on SD card when `log_to_file=1`).
- Read deltas per test run, not the full file.
- Interactive GDB: `docker-compose run --rm debugger <SWITCH_IP> [PID]`
- GDB component scripts in `scripts/debugger/components/` target specific subsystems: `ldn/`, `network/`, `config/`, `p2p/`, `bsd/`, `debug/`.
- Presets in `scripts/debugger/presets/`: `minimal.gdb`, `crash-analysis.gdb`, `ldn-focus.gdb`, `network-focus.gdb`.

### GDB Tracepoint Annotations

GDB dprintf tracepoints are defined as `@gdb{}` annotations inline in C++ headers, co-located with the functions they trace. A Python generator (`scripts/gdb_codegen.py`) extracts these and produces the `.gdb` files that `debug.sh` loads.

```cpp
/// @gdb{tag="LDN:LIFECYCLE", msg="Communication service created"}
explicit ICommunicationService(ncm::ProgramId program_id);

/// @gdb{tag="LDN:OPS", msg="Reject: nodeId=%u", args="$x1"}
Result Reject(u32 nodeId);
```

- **tag**: Hierarchical tag mapping to component + sub-file (e.g., `LDN:STATE` → `ldn/07-state.gdb`)
- **msg**: Printf format string without `[TAG]` prefix or `\n` (generator adds both)
- **args**: Optional GDB register arguments (ARM64 calling convention)

Commands:
- `python3 scripts/gdb_codegen.py generate` — regenerate `.gdb` files from annotations
- `python3 scripts/gdb_codegen.py verify` — compare annotations vs existing `.gdb` files
- `python3 scripts/gdb_codegen.py inject` — back-port `.gdb` entries into source headers

Documentation: `scripts/gdb_codegen/README.md`

## Docs Site

`docs/` is an Astro + Starlight site. Build with `npm run generate-api` (runs Doxygen, transforms XML → MDX) then `astro build`. Deploys to GitHub Pages.