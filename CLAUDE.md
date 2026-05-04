# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ryu_ldn_nx is an Atmosphere (Switch CFW) sysmodule that bridges Nintendo's LDN (Local Data Network) multiplayer to Ryujinx's LDN servers over TCP — replacing `ldn_mitm` + `lanplay` + `pcap` with a single zero-config sysmodule. It ships as TitleID `4200000000000010` (boot2).

The original `ryu_ldn` was designed for the Ryujinx emulator; this project ports it to run natively on Switch hardware, which means aggressively minimizing dynamic allocation, stack size, and thread count. All sizing decisions target the lowest-spec Switch model.

## Build & test commands

All builds run inside the devkitA64 Docker container defined in [Dockerfile](Dockerfile); use compose services rather than invoking `make` on the host.

```bash
# Sysmodule (.nsp) — depends on libstratosphere, which is built first automatically
docker-compose run --rm build

# Tesla overlay (.ovl)
docker-compose run --rm overlay

# Both, in parallel, waiting on file locks
docker-compose run --rm all

# Host unit tests (g++, not cross-compiled)
docker-compose run --rm test

# Clean sysmodule artifacts
docker-compose run --rm clean

# Interactive GDB against a running Switch — see scripts/debugger/debug.sh
docker-compose run --rm debugger <SWITCH_IP> [PID]

# Regenerate .gdb tracepoint files from @gdb{} annotations in source
python3 scripts/gdb_codegen.py generate       # produce .gdb files
python3 scripts/gdb_codegen.py verify          # compare annotations vs .gdb files
```

Unit tests live in [tests/](tests/) and build as native host binaries with `g++ -std=c++17 -DTEST_BUILD`. Each test suite has both an `all`/`test` aggregate target and a per-suite target. To run one suite:

```bash
cd tests && make test-ldn-state-machine       # any of: protocol, config, log, socket,
                                              # tcp-client, connection-state, reconnect,
                                              # client, ldn-types, ldn-state-machine,
                                              # ldn-proxy, ldn-error, ldn-integration,
                                              # overlay, ipc-config, config-ipc-service,
                                              # shared-state, packet-dispatcher,
                                              # session-handler, proxy-handler,
                                              # handler-integration, upnp, p2p-proxy,
                                              # p2p-client, p2p-integration,
                                              # p2p-create-network
make COVERAGE=1 coverage                      # gcov report
```

Dist packaging (SD-card-ready zip): `cd sysmodule && make dist` produces `ryu_ldn_nx-sysmodule.zip` with the `/atmosphere/contents/4200000000000010/` layout, including `boot2.flag`.

## Architecture — the big picture

The sysmodule registers **three** IPC services simultaneously from [main.cpp](sysmodule/source/main.cpp):

1. **`ldn:u` MITM** ([sysmodule/source/ldn/](sysmodule/source/ldn/)) — intercepts Nintendo's LDN user service. Implements `ICommunicationService` ([ldn_icommunication.cpp](sysmodule/source/ldn/ldn_icommunication.cpp)) with a `CommState` state machine (None → Initialized → AccessPoint/Station → AccessPointCreated/StationConnected). Commands like `CreateNetwork`, `Scan`, `Connect`, `OpenAccessPoint`, `SendTo` are forwarded to the Ryujinx server over TCP instead of onto Wi-Fi. The game-visible `NetworkInfo`, node IDs, and advertise data are synthesized from server responses.

2. **`bsd:u` MITM** ([sysmodule/source/bsd/](sysmodule/source/bsd/)) — intercepts BSD socket calls. Most calls are forwarded transparently to the real service; sockets that `bind`/`connect` to the LDN subnet **10.114.x.x** are tracked as proxy sockets and their traffic is tunneled through `ProxyData` packets. This is how the game's actual gameplay UDP/TCP traffic gets routed through Ryujinx servers without pcap. See [proxy_socket.cpp](sysmodule/source/bsd/proxy_socket.cpp) and [proxy_socket_manager.cpp](sysmodule/source/bsd/proxy_socket_manager.cpp).

3. **`ryu:cfg`** ([sysmodule/source/config/config_ipc_service.cpp](sysmodule/source/config/config_ipc_service.cpp)) — a custom (non-MITM) IPC service the Tesla overlay talks to for live config, status, and diagnostics.

These run on three independent thread pools (MITM server, config server, log-maintenance), all using a shared 96 KB expanded heap. Heap handle, `new`/`delete` overrides, and sizing constants are in [main.cpp](sysmodule/source/main.cpp) — **do not grow buffers casually**, total sysmodule budget is ~10 MB across all Switch sysmodules.

### Key cross-cutting modules

- [sysmodule/source/network/](sysmodule/source/network/) — `TcpClient` + `Client` wrap the BSD socket API to talk to the Ryujinx server. `ConnectionState` and `Reconnect` (with exponential backoff) sit on top.
- [sysmodule/source/protocol/](sysmodule/source/protocol/) — `ryu_protocol.hpp` / `packet_buffer.hpp` / `types.hpp`. **Wire format mirrors the C# server's `StructLayout` (no `Pack=1`)**: 12-byte `LdnHeader` with `DataSize` at offset 8 due to int32 alignment. The protocol is ported from `LdnServer/Network/RyuLdnProtocol.cs` — do not second-guess field padding here; it has been verified repeatedly against Ryujinx.
- [sysmodule/source/ldn/ldn_shared_state.cpp](sysmodule/source/ldn/ldn_shared_state.cpp) — shared between `ICommunicationService` and `ConfigService` so the overlay can display real-time LDN status.
- [sysmodule/source/ldn/ldn_state_machine.cpp](sysmodule/source/ldn/ldn_state_machine.cpp), [ldn_packet_dispatcher.cpp](sysmodule/source/ldn/ldn_packet_dispatcher.cpp), [ldn_session_handler.cpp](sysmodule/source/ldn/ldn_session_handler.cpp), [ldn_proxy_handler.cpp](sysmodule/source/ldn/ldn_proxy_handler.cpp) — the LDN request flow is split into these pieces: state-machine enforces transitions, dispatcher routes incoming packets, handlers implement session/proxy logic.
- [sysmodule/source/p2p/](sysmodule/source/p2p/) — optional P2P proxy path using UPnP port mapping (`-lminiupnpc`) to reduce server load once peers discover each other.
- [sysmodule/source/config/](sysmodule/source/config/) — fixed-buffer INI parser (no heap), default-on-failure semantics, plus a baked-in game whitelist (~40 KB, which is why the heap is sized at 96 KB).
- [sysmodule/source/debug/log.cpp](sysmodule/source/debug/log.cpp) — file logger with an idle-timeout thread that closes the handle after 2 s of inactivity.

### Tesla overlay

[overlay/](overlay/) is a separate build producing `.ovl`, using `libultrahand`. It talks to `ryu:cfg` via the IPC stubs in [overlay/source/ryu_ldn_ipc.c](overlay/source/ryu_ldn_ipc.c). Overlay behavior is unit-tested through host mocks in [tests/overlay_tests.cpp](tests/overlay_tests.cpp).

### Docs site

[docs/](docs/) is an Astro + Starlight site. `npm run generate-api` runs Doxygen (via [Doxyfile](Doxyfile)) and transforms XML into MDX before `astro build`. The site ships to GitHub Pages.

## Conventions specific to this project

- **Wire format / padding**: Per [PROMPT.md](PROMPT.md) (in French), the packet layout — including implicit alignment padding inherited from C#'s default `StructLayout` — is load-bearing and non-negotiable. If a bug looks like a protocol-padding mismatch, investigate elsewhere first (game behavior, proxy socket routing, state machine transitions). Questioning the padding without concrete evidence from Ryujinx source will waste a cycle.
- **Reference sources** the author expects Claude to cross-check live from `~/GIT`: `ryujinx/` (emulator C# client), `ldn/` (Ryujinx LDN server), `ldn_mitm/` (original Switch sysmodule), plus switchbrew.org and NintendoClients wiki.
- **No dynamic allocation** in hot paths. Fixed buffers, `constinit` statics, stack-allocated work areas. `new`/`delete` route to a 96 KB expanded heap — if you're tempted to grow it, prove it's needed.
- **Commits must be DCO-signed** (`git commit -s`). The project enforces this via CONTRIBUTING.md / PR checks.
- **IDE diagnostics are expected to be noisy** because devkitPro only exists inside the build container — don't spend tokens explaining why the host IDE can't find `<stratosphere.hpp>`.
- **Switch logs for debugging**: after each on-console test the log is dropped into [tmp/logs/ryu_ldn_nx.log](tmp/logs/ryu_ldn_nx.log). Read deltas per run rather than re-parsing the full file. On the Switch side the log lives at `config/ryu_ldn_nx/ryu_ldn_nx.log` on the SD card.
