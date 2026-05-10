# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ryu_ldn_nx is an Atmosphere (Switch CFW) sysmodule that bridges Nintendo's LDN (Local Data Network) multiplayer to Ryujinx's LDN servers over TCP — replacing `ldn_mitm` + `lanplay` + `pcap` with a single zero-config sysmodule. It ships as TitleID `4200000000000010` (boot2).

The original `ryu_ldn` was designed for the Ryujinx emulator; this project ports it to run natively on Switch hardware, which means aggressively minimizing dynamic allocation, stack size, and thread count. All sizing decisions target the lowest-spec Switch model.

## Build & test commands

All builds run inside the devkitA64 Docker container defined in [Dockerfile](Dockerfile); use compose services rather than invoking `make` on the host.

```bash
# Sysmodule + overlay + dist ZIP (single command)
docker compose run --rm build

# Host unit tests (g++, not cross-compiled)
docker compose run --rm test

# Clean all build artifacts and output/
docker compose run --rm clean

# Interactive GDB against a running Switch
docker compose run --rm debugger <SWITCH_IP> [PID]

# Regenerate .gdb tracepoint files from @gdb{} annotations in source
python3 scripts/gdb_codegen.py generate
python3 scripts/gdb_codegen.py verify
```

Unit tests live in [tests/](tests/) and build as native host binaries with `g++ -std=c++17 -DTEST_BUILD`. Each test suite has a per-suite target:

```bash
cd tests && make test-ldn-state-machine       # any of: protocol, config, log, socket,
                                              # tcp-client, connection-state, reconnect,
                                              # client, ldn-types, ldn-state-machine,
                                              # ldn-proxy, ldn-error, ldn-integration, ...
make COVERAGE=1 coverage
```

Dist packaging: `cd sysmodule && make dist` produces `output/` with SD card layout and `ryu_ldn_nx-release.zip`.

## Architecture — the big picture

The sysmodule registers **three** IPC services simultaneously from [main.cpp](sysmodule/source/main.cpp):

1. **`ldn:u` MITM** ([sysmodule/source/ldn/](sysmodule/source/ldn/)) — intercepts Nintendo's LDN user service. Implements `ICommunicationService` ([ldn_icommunication.cpp](sysmodule/source/ldn/ldn_icommunication.cpp)) with a `CommState` state machine (None → Initialized → AccessPoint/Station → AccessPointCreated/StationConnected). Commands like `CreateNetwork`, `Scan`, `Connect`, `OpenAccessPoint`, `SendTo` are forwarded to the Ryujinx server over TCP instead of onto Wi-Fi. The game-visible `NetworkInfo`, node IDs, and advertise data are synthesized from server responses.

2. **`bsd:u` MITM** ([sysmodule/source/bsd/](sysmodule/source/bsd/)) — intercepts BSD socket calls. Most calls are forwarded transparently to the real service; sockets that `bind`/`connect` to the LDN subnet **10.114.x.x** are tracked as proxy sockets and their traffic is tunneled through `ProxyData` packets. This is how gameplay UDP/TCP traffic gets routed through Ryujinx servers without pcap. See [proxy_socket.cpp](sysmodule/source/bsd/proxy_socket.cpp) and [proxy_socket_manager.cpp](sysmodule/source/bsd/proxy_socket_manager.cpp).

3. **`ryu:cfg`** ([sysmodule/source/config/config_ipc_service.cpp](sysmodule/source/config/config_ipc_service.cpp)) — custom (non-MITM) IPC service the Tesla overlay talks to for live config, status, and diagnostics.

These run on three independent thread pools (MITM server, config server, log-maintenance), all using a shared 384 KB expanded heap. Heap handle, `new`/`delete` overrides, and sizing constants are in [main.cpp](sysmodule/source/main.cpp) — **do not grow buffers casually**, total sysmodule budget is ~10 MB across all Switch sysmodules.

### PIA Protocol (game-level mesh networking)

Games on Switch use Nintendo's PIA library on top of LDN for peer-to-peer mesh networking. After `CreateNetwork`/`Connect`:

1. Game calls `GetIpv4Address()` → gets virtual LDN IP (e.g., `10.114.0.1`)
2. Game calls `GetNetworkInfo()` → gets `NetworkInfo` with all nodes' IPs/MACs
3. Game identifies its own node by matching `GetIpv4Address()` result against `NetworkInfo.ldn.nodes[].ipv4Address`
4. Game opens UDP sockets via `bsd:u` → MITM intercepts `bind`/`connect` to `10.114.x.x` → creates `ProxySocket`
5. PIA sends broadcast UDP packets (mesh discovery) → `ProxySocketManager::RouteIncomingData()` must deliver to **all** matching sockets
6. PIA sends unicast UDP/TCP packets → `ProxySocket` → `ProxyData` header → TCP tunnel → server → peer

**Critical**: `FindAllSocketsByDestination()` delivers broadcast packets to all listening proxy sockets on the same port. Single-delivery (`FindSocketByDestination`) broke PIA mesh discovery in games like Smash Bros.

### Packet signaling in WaitForResponse

`HandleServerPacket` signals `m_response_event` **only** for actual response packets that `WaitForResponse` expects: `Connected`, `ScanReply`, `ScanReplyEnd`, `RejectReply`, `ProxyConnectReply`, `NetworkError`. Async packets (`ProxyConfig`, `ProxyData`, `ExternalProxy`, `SyncNetwork`, `Ping`) do **not** signal the event. This prevents spurious wake-ups that previously caused false timeout errors.

### Key cross-cutting modules

- [sysmodule/source/network/](sysmodule/source/network/) — `TcpClient` + `Client` wrap the BSD socket API to talk to the Ryujinx server. `ConnectionState` and `ReconnectManager` (with fast first retry + exponential backoff + jitter) sit on top. TCP keepalive is enabled (30s/10s/5 probes).
- [sysmodule/source/protocol/](sysmodule/source/protocol/) — `ryu_protocol.hpp` / `packet_buffer.hpp` / `types.hpp`. **Wire format mirrors the C# server's `StructLayout` (no `Pack=1`)**: 12-byte `LdnHeader` with `DataSize` at offset 8 due to int32 alignment. The protocol is ported from `LdnServer/Network/RyuLdnProtocol.cs` — do not second-guess field padding here; it has been verified repeatedly against Ryujinx.
- [sysmodule/source/ldn/ldn_shared_state.cpp](sysmodule/source/ldn/ldn_shared_state.cpp) — shared between `ICommunicationService` and `ConfigService` so the overlay can display real-time LDN status.
- [sysmodule/source/ldn/ldn_icommunication.cpp](sysmodule/source/ldn/ldn_icommunication.cpp) — main LDN service: CreateNetwork, Connect, Scan, WaitForResponse, HandleServerPacket, FindLocalNodeId. **The `m_network_info` race**: receive thread writes, IPC thread reads — must hold `m_shared_mutex`.
- [sysmodule/source/bsd/proxy_socket_manager.cpp](sysmodule/source/bsd/proxy_socket_manager.cpp) — proxy data routing. `RouteIncomingData()` uses `FindAllSocketsByDestination()` for broadcast fan-out to all matching proxy sockets.
- [sysmodule/source/p2p/](sysmodule/source/p2p/) — optional P2P proxy path using UPnP port mapping (`-lminiupnpc`) to reduce server load once peers discover each other.
- [sysmodule/source/config/](sysmodule/source/config/) — fixed-buffer INI parser (no heap), default-on-failure semantics, plus a baked-in game whitelist (~40 KB).

### Byte order — critical invariant

LDN IPs in `NetworkInfo`, `ProxyConfig`, `m_ipv4_address`, and `GetIpv4Address()` are all in **Ryujinx format** — big-endian read as `uint32` (e.g., `10.114.0.1` = `0x0A720001`). BSD `sockaddr_in.sin_addr` uses **network byte order**. `GetAddr()` does `bswap32`, `sin_addr` is stored in Ryujinx format (no bswap in `Bind`). Never double-convert.

### Tesla overlay

[overlay/](overlay/) is a separate build producing `.ovl`, using `libultrahand`. It talks to `ryu:cfg` via the IPC stubs in [overlay/source/ryu_ldn_ipc.c](overlay/source/ryu_ldn_ipc.c).

### Docs site

[docs/](docs/) is an Astro + Starlight site. `npm run generate-api` runs Doxygen (via [Doxyfile](Doxyfile)) and transforms XML into MDX before `astro build`. The site ships to GitHub Pages.

## Conventions specific to this project

- **Wire format / padding**: The packet layout — including implicit alignment padding inherited from C#'s default `StructLayout` — is load-bearing and non-negotiable. If a bug looks like a protocol-padding mismatch, investigate elsewhere first (game behavior, proxy socket routing, state machine transitions). Questioning the padding without concrete evidence from Ryujinx source will waste a cycle.
- **Reference sources** to cross-check live from `~/GIT`: `ryujinx/` (emulator C# client), `ldn/` (Ryujinx LDN server), `ldn_mitm/` (original Switch sysmodule), plus switchbrew.org and NintendoClients wiki.
- **No dynamic allocation** in hot paths. Fixed buffers, `constinit` statics, stack-allocated work areas. `new`/`delete` route to a 384 KB expanded heap — if you're tempted to grow it, prove it's needed.
- **Commits must be DCO-signed** (`git commit -s`). Use `-s` so the `Signed-off-by:` trail comes from git config — **never hardcode a developer's name or email**.
- **IDE diagnostics are expected to be noisy** because devkitPro only exists inside the build container — don't spend tokens explaining why the host IDE can't find `<stratosphere.hpp>`.
- **Switch logs for debugging**: after each on-console test the log is dropped into [tmp/logs/ryu_ldn_nx.log](tmp/logs/ryu_ldn_nx.log). Read deltas per run rather than re-parsing the full file. On the Switch side the log lives at `config/ryu_ldn_nx/ryu_ldn_nx.log` on the SD card.

## Known issues & pitfalls

- **`FindLocalNodeId()` returns `0xFF`** (255) when the local IP doesn't match any node — not `0`. Node index 0 is a valid host ID. A return of `0xFF` means "not found".
- **P2P timing**: In P2P mode, `CreateNetwork` pre-sets `m_ipv4_address = 0x0A720001` (predicted host IP) before the P2P worker finishes. The `Connected` handler on the receive thread may call `FindLocalNodeId()` before `m_ipv4_address` is updated.
- **Relay mode timing**: In relay mode, `ProxyConfig` arrives before `Connected`. Only response-type packets signal `m_response_event` (see Packet Signaling above). `ProxyConfig` no longer wakes `WaitForResponse`.
- **`m_network_info` race**: The receive thread writes `m_network_info` via `HandleServerPacket`. Any IPC handler reading it must hold `m_shared_mutex`.
- **`ping_interval` is unused**: The `ping_interval` config key is parsed and stored but never consumed by the network client.
- **`[perf]` section is dead**: The perf config keys are in the example config but not parsed or consumed. They have zero runtime effect.
- **`max_reconnect_attempts = 0` disables auto-reconnect**: It does not mean infinite retries — it means the client will not retry at all.