/**
 * @file dns_wrap.cpp
 * @brief Linker wrappers for resolver functions that crash in our sysmodule
 *
 * libnx's getaddrinfo() / gethostbyname() / getnameinfo() route to the
 * `sfdnsres` system service. From the boot2 sysmodule context (TitleID
 * 0x4200000000000010) those calls reliably DABRT — likely because of a
 * resource-limit / capability mismatch around the way the service hands
 * out a TransferMemory-backed buffer. Diagnosed in upnp_port_mapper's
 * BSD pre-flight test where `getaddrinfo("239.255.255.250","1900",AF_INET)`
 * never returned and never logged a result line — the syscall chain
 * panics inside libnx before getting back to us.
 *
 * This compilation unit defines `__wrap_*` versions and the Makefile
 * adds `-Wl,--wrap=getaddrinfo,--wrap=freeaddrinfo,--wrap=getnameinfo,--wrap=gai_strerror`.
 * The linker redirects every call (including those baked into
 * `libminiupnpc.a`) to these implementations, so miniupnpc's SSDP
 * discovery + HTTP control flow can run without ever touching sfdnsres.
 *
 * **Scope of substitution**: only IPv4 numeric literals are supported.
 * For UPnP that is a non-issue: SSDP targets the fixed multicast address
 * `239.255.255.250` and routers always advertise their LOCATION as an
 * IPv4-literal URL. Hostname resolution remains intentionally
 * unsupported (returns `EAI_NONAME`) — the rest of the sysmodule already
 * pre-resolves IP literals via `inet_pton` in `network/socket.cpp`.
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {

int __wrap_getaddrinfo(const char* node, const char* service,
                       const struct addrinfo* hints,
                       struct addrinfo** res) {
    if (!res) {
        errno = EINVAL;
        return EAI_SYSTEM;
    }
    *res = nullptr;

    if (!node && !service) {
        return EAI_NONAME;
    }

    int family = hints ? hints->ai_family : AF_UNSPEC;
    if (family != AF_UNSPEC && family != AF_INET) {
        return EAI_FAMILY;
    }

    uint16_t port = 0;
    if (service) {
        port = static_cast<uint16_t>(std::atoi(service));
    }

    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    if (node) {
        if (::inet_pton(AF_INET, node, &sa.sin_addr) != 1) {
            return EAI_NONAME;
        }
    } else {
        const bool passive = hints && (hints->ai_flags & AI_PASSIVE);
        sa.sin_addr.s_addr = htonl(passive ? INADDR_ANY : INADDR_LOOPBACK);
    }

    auto* ai = static_cast<struct addrinfo*>(
        std::malloc(sizeof(struct addrinfo)));
    if (!ai) return EAI_MEMORY;
    auto* sa_buf = static_cast<struct sockaddr_in*>(
        std::malloc(sizeof(struct sockaddr_in)));
    if (!sa_buf) {
        std::free(ai);
        return EAI_MEMORY;
    }

    std::memset(ai, 0, sizeof(struct addrinfo));
    std::memcpy(sa_buf, &sa, sizeof(sa));
    ai->ai_family   = AF_INET;
    ai->ai_socktype = hints ? hints->ai_socktype : 0;
    ai->ai_protocol = hints ? hints->ai_protocol : 0;
    ai->ai_addrlen  = sizeof(struct sockaddr_in);
    ai->ai_addr     = reinterpret_cast<struct sockaddr*>(sa_buf);
    ai->ai_canonname = nullptr;
    ai->ai_next      = nullptr;

    *res = ai;
    return 0;
}

void __wrap_freeaddrinfo(struct addrinfo* res) {
    while (res) {
        struct addrinfo* next = res->ai_next;
        std::free(res->ai_addr);
        std::free(res);
        res = next;
    }
}

int __wrap_getnameinfo(const struct sockaddr* sa, socklen_t salen,
                       char* host, socklen_t hostlen,
                       char* serv, socklen_t servlen, int /*flags*/) {
    if (!sa || salen < static_cast<socklen_t>(sizeof(struct sockaddr_in))) {
        return EAI_FAMILY;
    }
    if (sa->sa_family != AF_INET) {
        return EAI_FAMILY;
    }

    const auto* sin = reinterpret_cast<const struct sockaddr_in*>(sa);

    if (host && hostlen > 0) {
        if (::inet_ntop(AF_INET, &sin->sin_addr, host, hostlen) == nullptr) {
            return EAI_OVERFLOW;
        }
    }
    if (serv && servlen > 0) {
        std::snprintf(serv, servlen, "%u", ntohs(sin->sin_port));
    }
    return 0;
}

const char* __wrap_gai_strerror(int /*ecode*/) {
    return "dns error";
}

} // extern "C"
