# Guía Rápida de Uso — ryu_ldn_nx

**ryu_ldn_nx** es un sysmodule de Atmosphere que te permite jugar partidas con el modo "Inalámbrico Local" de tus juegos de Switch a través de los servidores de Ryujinx LDN por Internet, de forma directa y sin necesidad de usar programas puentes en una PC.

---

## 📋 Requisitos de la Switch
1. **Custom Firmware:** Nintendo Switch con **Atmosphere CFW** instalado.
2. **Conexión a Internet:** Conexión WiFi activa en la Switch.
3. **Servidor Libre:** Que el dominio `ldn.ryujinx.app` sea accesible (si usas bloqueadores DNS como 90DNS, asegúrate de no bloquearlo).

---

## ⚠️ Incompatibilidades Críticas (Limpieza obligatoria)
Para que el sistema funcione correctamente y no cause bloqueos en el arranque (pantallazos negros/azules o kernel panic), **debes eliminar los siguientes módulos antes de instalar**:

1. **Eliminar `ldn_mitm` estándar:**
   * Comparten el mismo identificador de programa (TitleID: `4200000000000010`).
   * **Acción:** Borra por completo la carpeta `sdmc:/atmosphere/contents/4200000000000010/` de tu tarjeta SD.
2. **Eliminar `switch-lan-play` / `lanplay-pc-less` (Sysmodule):**
   * El módulo de lan-play (TitleID: `42000000000000B1`) causará conflictos de red e interceptación de sockets.
   * **Acción:** Borra por completo la carpeta `sdmc:/atmosphere/contents/42000000000000B1/` de tu tarjeta SD.

---

## 🚀 Instalación Rápida
1. Descarga el archivo de distribución (`ryu_ldn_nx-release.zip`).
2. Extrae el contenido directamente en la **raíz de tu tarjeta SD** (fusionando la carpeta `atmosphere`).
3. Reinicia tu Nintendo Switch.

---

## ⚙️ Configuración Básica
En el primer arranque, el sysmodule creará automáticamente el archivo de configuración en:
📂 `sdmc:/config/ryu_ldn_nx/config.ini`

Los valores por defecto apuntan al servidor público oficial de Ryujinx (`ldn.ryujinx.app` en el puerto `30456`). Si deseas configurar una sala privada, edita este archivo e introduce tu contraseña en `passphrase = tu_clave` bajo la sección `[ldn]`.
