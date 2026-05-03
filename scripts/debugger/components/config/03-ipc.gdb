# ==========================================
# Config Component - IPC Service
# ==========================================
# Configuration IPC service operations
# Namespace: ryu_ldn::ipc::ConfigService
# Using dprintf for automatic continue

echo [CONFIG] Loading IPC service breakpoints...\n

# Version and status
dprintf ryu_ldn::ipc::ConfigService::GetVersion, "[CONFIG:IPC] GetVersion\n"
dprintf ryu_ldn::ipc::ConfigService::GetConnectionStatus, "[CONFIG:IPC] GetConnectionStatus\n"
dprintf ryu_ldn::ipc::ConfigService::IsServiceActive, "[CONFIG:IPC] IsServiceActive\n"

# Passphrase
dprintf ryu_ldn::ipc::ConfigService::GetPassphrase, "[CONFIG:IPC] GetPassphrase\n"
dprintf ryu_ldn::ipc::ConfigService::SetPassphrase, "[CONFIG:IPC] SetPassphrase\n"

# Server address (combined host+port)
dprintf ryu_ldn::ipc::ConfigService::GetServerAddress, "[CONFIG:IPC] GetServerAddress\n"
dprintf ryu_ldn::ipc::ConfigService::SetServerAddress, "[CONFIG:IPC] SetServerAddress\n"

# LDN settings
dprintf ryu_ldn::ipc::ConfigService::GetLdnEnabled, "[CONFIG:IPC] GetLdnEnabled\n"
dprintf ryu_ldn::ipc::ConfigService::SetLdnEnabled, "[CONFIG:IPC] SetLdnEnabled\n"

# TLS
dprintf ryu_ldn::ipc::ConfigService::GetUseTls, "[CONFIG:IPC] GetUseTls\n"
dprintf ryu_ldn::ipc::ConfigService::SetUseTls, "[CONFIG:IPC] SetUseTls\n"

# Debug settings
dprintf ryu_ldn::ipc::ConfigService::GetDebugEnabled, "[CONFIG:IPC] GetDebugEnabled\n"
dprintf ryu_ldn::ipc::ConfigService::SetDebugEnabled, "[CONFIG:IPC] SetDebugEnabled\n"
dprintf ryu_ldn::ipc::ConfigService::GetDebugLevel, "[CONFIG:IPC] GetDebugLevel\n"
dprintf ryu_ldn::ipc::ConfigService::SetDebugLevel, "[CONFIG:IPC] SetDebugLevel\n"
dprintf ryu_ldn::ipc::ConfigService::GetLogToFile, "[CONFIG:IPC] GetLogToFile\n"
dprintf ryu_ldn::ipc::ConfigService::SetLogToFile, "[CONFIG:IPC] SetLogToFile\n"

# Network settings
dprintf ryu_ldn::ipc::ConfigService::GetConnectTimeout, "[CONFIG:IPC] GetConnectTimeout\n"
dprintf ryu_ldn::ipc::ConfigService::SetConnectTimeout, "[CONFIG:IPC] SetConnectTimeout\n"
dprintf ryu_ldn::ipc::ConfigService::GetPingInterval, "[CONFIG:IPC] GetPingInterval\n"
dprintf ryu_ldn::ipc::ConfigService::SetPingInterval, "[CONFIG:IPC] SetPingInterval\n"

# Save/Load
dprintf ryu_ldn::ipc::ConfigService::SaveConfig, "[CONFIG:IPC] SaveConfig\n"
dprintf ryu_ldn::ipc::ConfigService::ReloadConfig, "[CONFIG:IPC] ReloadConfig\n"

# Live diagnostics
dprintf ryu_ldn::ipc::ConfigService::IsGameActive, "[CONFIG:IPC] IsGameActive\n"
dprintf ryu_ldn::ipc::ConfigService::GetLdnState, "[CONFIG:IPC] GetLdnState\n"
dprintf ryu_ldn::ipc::ConfigService::GetSessionInfo, "[CONFIG:IPC] GetSessionInfo\n"
dprintf ryu_ldn::ipc::ConfigService::GetLastRtt, "[CONFIG:IPC] GetLastRtt\n"
dprintf ryu_ldn::ipc::ConfigService::ForceReconnect, "[CONFIG:IPC] ForceReconnect\n"
dprintf ryu_ldn::ipc::ConfigService::GetActiveProcessId, "[CONFIG:IPC] GetActiveProcessId\n"

# P2P setting
dprintf ryu_ldn::ipc::ConfigService::GetDisableP2p, "[CONFIG:IPC] GetDisableP2p\n"
dprintf ryu_ldn::ipc::ConfigService::SetDisableP2p, "[CONFIG:IPC] SetDisableP2p\n"

echo [CONFIG] IPC Service: 31 dprintf points\n