# ryu_ldn_nx

[![Build Status](https://github.com/shaklinedj/ryu_ldn_nx/actions/workflows/build.yml/badge.svg)](https://github.com/shaklinedj/ryu_ldn_nx/actions)
[![Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/Pikatsuto/4277995494c6874ebb238bdbc499d9d0/raw/coverage.json)](https://github.com/shaklinedj/ryu_ldn_nx/actions)
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](https://www.gnu.org/licenses/gpl-2.0)

Un sysmodule para Nintendo Switch que habilita el multijugador online a través de los servidores LDN de Ryujinx, sin necesidad de configuraciones de red complejas.

## 🚧 Trabajo en Progreso

Este proyecto está en desarrollo activo. La resolución de nombres de host (DNS) y la encriptación TLS están completamente implementadas, permitiendo una conexión segura y directa a los servidores LDN oficiales de Ryujinx (`ldn.ryujinx.app`). Compatible con el último Firmware 22.5.0 y Atmosphere 1.11.2 (con retrocompatibilidad).

## ¿Qué es ryu_ldn_nx?

ryu_ldn_nx es un sysmodule de Atmosphere que intercepta el tráfico de los juegos con conexión inalámbrica local (LDN) y lo redirige a través de los servidores LDN de Ryujinx mediante TCP. Esto permite a los usuarios de Switch con Custom Firmware (CFW) jugar en línea sin necesidad de:

- Configuraciones complejas de juego LAN
- PC con pcap en modo de conexión compartida
- Configuración manual de red

¡Solo instala, conéctate al WiFi y juega!

## Cómo funciona

ryu_ldn_nx registra **tres servicios IPC** simultáneamente:

1. **MITM de `ldn:u`** — Intercepta el servicio de usuario LDN de Nintendo. Implementa `ICommunicationService` con una máquina de estados completa. Los juegos ven la red (`NetworkInfo`) y los IDs de nodos sintetizados a partir de las respuestas del servidor de Ryujinx.

2. **MITM de `bsd:u`** — Intercepta las llamadas a sockets BSD. Los sockets que se vinculan (`bind`/`connect`) a la subred LDN `10.114.x.x` son rastreados y su tráfico se encapsula en paquetes `ProxyData` hacia el servidor de Ryujinx.

3. **`ryu:cfg`** — Servicio IPC personalizado con el que se comunica el menú de Tesla (overlay) para mostrar la configuración en vivo y el estado de la conexión.

## Funciones Planeadas

- **Plug and Play**: Se instala como cualquier sysmodule, sin requerir configuración adicional.
- **Conexión Automática**: Se conecta a los servidores de Ryujinx automáticamente.
- **Todos los juegos LDN**: Funciona con cualquier juego que soporte el modo multijugador local inalámbrico.
- **Menú Tesla (Overlay)**: Estado en vivo, configuración y gestión de la conexión.
- **Soporte P2P**: Conexión directa opcional de igual a igual (Peer-to-Peer) mediante UPnP para reducir la carga del servidor.

## Instalación

### Requisitos

- Nintendo Switch con Atmosphere CFW (1.0.0 o superior)
- Conexión a Internet (WiFi)
- Tarjeta SD con Atmosphere instalado

### Instalación Rápida

1. Descarga la última versión (Release) desde la pestaña [Releases](https://github.com/shaklinedj/ryu_ldn_nx/releases).
2. Extrae el archivo ZIP en la raíz de tu tarjeta SD (contiene la estructura `/atmosphere/contents/4200000000000010/`).
3. Reinicia tu consola Switch.
4. Inicia cualquier juego con modo multijugador local: ¡automáticamente usará el modo en línea!

### Configuración (Opcional)

Crea el archivo `sdmc:/config/ryu_ldn_nx/config.ini` (o deja que el sysmodule lo cree automáticamente en el primer arranque):

```ini
[server]
host = ldn.ryujinx.app    ; Dirección IP o nombre de host
port = 30456
use_tls = 1               ; Usar encriptación TLS (mbedtls) para una conexión segura

[network]
connect_timeout = 5000
reconnect_delay = 3000
max_reconnect_attempts = 5       ; 0 = desactivar autoconexión

[ldn]
enabled = 1
disable_p2p = 1

[debug]
enabled = 0
level = 1
log_to_file = 0
```

Todos los ajustes tienen valores por defecto lógicos y se pueden cambiar en vivo mediante el menú Tesla.

## Juegos Soportados

Cualquier juego que soporte el modo inalámbrico local (LDN) de Nintendo debería funcionar, incluyendo:

- Mario Kart 8 Deluxe
- Super Smash Bros. Ultimate
- Splatoon 2 / 3
- Monster Hunter Rise
- Animal Crossing: New Horizons
- Pokémon Sword / Shield
- ¡Y muchos más!

## Desarrollo

### Compilar desde el código fuente

#### Usando Docker (Recomendado)

```bash
git clone --recursive https://github.com/shaklinedj/ryu_ldn_nx.git
cd ryu_ldn_nx

# Compilar todo (sysmodule + overlay + archivo ZIP)
docker compose run --rm build

# Ejecutar pruebas unitarias locales
docker compose run --rm test

# Limpiar los archivos generados
docker compose run --rm clean

# Empaquetar para distribución
cd sysmodule && make dist
```

#### Construcción Nativa

Requisitos:
- [devkitPro](https://devkitpro.org/wiki/Getting_Started) con devkitA64
- libnx
- Librerías de Atmosphere (libstratosphere)

```bash
dkp-pacman -S switch-dev libnx
git clone --recursive https://github.com/Atmosphere-NX/Atmosphere.git
cd Atmosphere && make -C libraries

export ATMOSPHERE_LIBS=/ruta/a/Atmosphere/libraries
cd /ruta/a/ryu_ldn_nx/sysmodule && make
```

### Depuración en Hardware (Debugging)

```bash
# Sesión interactiva de GDB
docker compose run --rm debugger <SWITCH_IP> [PID]
```

Archivos de registro (logs) en la Switch: `config/ryu_ldn_nx/ryu_ldn_nx.log` (cuando `log_to_file=1`).

## Licencia

Este proyecto está bajo la Licencia Pública General de GNU v2.0 (GPL v2). Consulta el archivo [LICENSE](LICENSE) para más detalles.

## Créditos

- [ldn_mitm](https://github.com/spacemeowx2/ldn_mitm) — Implementación original de LDN para Switch.
- [Ryujinx](https://git.ryujinx.app/ryubing/ryujinx) — Servidor y protocolo LDN.
- [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) — CFW de Switch y librerías base.
- [libnx](https://github.com/switchbrew/libnx) — Librería para homebrew de Switch.

## Descargo de Responsabilidad

Este software se proporciona "tal cual", sin garantía de ningún tipo. Úsalo bajo tu propia responsabilidad. Este proyecto no está afiliado a Nintendo, Ryujinx, ni Atmosphere.