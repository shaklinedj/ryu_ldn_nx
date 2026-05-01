# P2P with a self-hosted RyuLDN server on the same LAN

## Status

Known limitation of the **test setup**, not a sysmodule bug. Documented so a
contributor running their own RyuLDN server on their LAN doesn't burn an
afternoon thinking the sysmodule is broken.

## Symptom

You stand up your own copy of `~/GIT/ldn` (or `~/GIT/ryuldn-server`) on a PC
that sits **on the same LAN as the Switch hosting the network**, and you
configure the Switch to talk to it via the box's **public IP** (typically to
mirror what a real "remote server" deployment looks like, or to avoid
polluting the official RyuLDN production server during testing).

The host's `Création de partie privée` flow starts P2P, opens UPnP, sends
`CreateAccessPoint` with `ExternalProxyPort = 39990`, and the server
immediately replies:

```
Received NetworkError: PortUnreachable — disabling P2P (cleanup deferred)
```

The session falls back to relay mode and a joiner can still connect via the
master server. P2P never engages.

When the *same* sysmodule build is pointed at the official production
RyuLDN server (or any server reachable through real Internet routing), the
flow goes all the way to P2P:

```
Received ExternalProxyToken: virtual_ip=0x0A720001
Received ExternalProxy: port=39990
P2P connection from <server-IP>:42490
```

…and a Ryujinx joiner can dial the host through the UPnP-mapped public port.

## Why it happens

This is **not** a bug in `ryu_ldn_nx`, in the RyuLDN server, or in Ryujinx.
The wire payload from the sysmodule is byte-identical between the two runs
(`AddressFamily = 2`, `ExternalProxyPort = 0x9C36 = 39990`, `PrivateIp =
192.168.1.x` in network byte order, etc. — see the `send_create_access_point
wire [...]` rows in any host log).

The difference is in **how the box routes** the `IsProxyReachable` test
that the server fires (`TcpClient.ConnectAsync(Socket.RemoteEndPoint.Address,
ExternalProxyPort)` for 2 s, see
`~/GIT/ldn/LdnServer/Session/LdnSession.cs::IsProxyReachable`):

- **Production / Internet-hosted server:** the Switch's master TCP egresses
  through the box (NAT to public IP), reaches the server. Server stores the
  remote endpoint as `82.67.118.146` (the box's WAN IP). It connects back
  to `82.67.118.146:39990`, which the box DNATs (UPnP) onto
  `Switch_LAN:39990`. **Symmetric, predictable, works.**
- **Self-hosted server on the same LAN, contacted via the box's public
  IP:** the Switch's master TCP packet hits the box, the box sees the
  destination is itself and "hairpin"s it back inside to the LAN-side
  server. What the server sees as `Socket.RemoteEndPoint.Address` then
  depends entirely on whether the box rewrites the source during the
  hairpin (some do, some don't, some only do for static port forwards but
  not for UPnP-installed ones). When the server then runs `IsProxyReachable`
  it talks to that perceived address, and on many residential gateways the
  hairpin path back **to a UPnP-mapped port** behaves differently from the
  hairpin path **to a fixed port forward** — the SYN never reaches the
  Switch's `accept()`. Hence `PortUnreachable`.

## How to test the sysmodule's P2P path correctly

Pick *one* of:

1. **Use the official production RyuLDN server.** The most realistic test —
   it's how end users hit it. Be reasonable with the load. The host log
   should show `Received ExternalProxyToken` followed by `P2P connection
   from <server>:port` within a few hundred ms of `send_create_access_point`.
2. **Self-host the server on a machine outside your LAN** (a VPS, a cheap
   cloud instance, a friend's network). The Switch's master TCP then takes
   a real Internet path and the symmetry is restored.
3. **Self-host on the same LAN but point the Switch at the server's LAN
   IP**, not the box's public IP. The server then sees the Switch as
   `192.168.1.x` directly and `IsProxyReachable` does a direct LAN
   `connect()`. This works around the hairpin entirely. (The downside is
   that you're not testing the production-shaped wire path; that's why
   we use `(2)` when we want both privacy and realism.)

## What this is **not**

- It is **not** a sysmodule bug; the same .nsp works against production
  with the exact same payload.
- It is **not** a server bug; `IsProxyReachable` has been doing this on
  the upstream RyuLDN server for years across countless networks.
- It is **not** a UPnP bug; the box returns `200 OK` on `AddPortMapping`
  and the production-path test confirms the mapping is effective from
  the Internet side.

## Tracking

If you observe this symptom on a setup that is *not* the same-LAN-via-public-IP
case (i.e. you point at a real Internet-side server and still get
`PortUnreachable`), that is a different bug and worth investigating. File
an issue with the host log and the routing setup.
