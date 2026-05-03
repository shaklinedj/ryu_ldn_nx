# ==========================================
# LDN Component - Shared State & State Machine
# ==========================================
# LdnStateMachine, SharedState, and ICommunicationService
# state management. Updated for async architecture:
# LdnStateMachine::TransitionTo, SignalStateChange,
# SharedState with thread-safe state access.
# Using dprintf for automatic continue

echo [LDN] Loading shared state and state machine breakpoints...\n

# ==========================================
# LdnStateMachine — CommState transitions
# (ams::mitm::ldn namespace)
# ==========================================

dprintf ams::mitm::ldn::LdnStateMachine::LdnStateMachine, "[LDN:STATE] LdnStateMachine: constructor\n"
dprintf ams::mitm::ldn::LdnStateMachine::~LdnStateMachine, "[LDN:STATE] LdnStateMachine: destructor\n"

# State transitions — the core of the CommState machine
dprintf ams::mitm::ldn::LdnStateMachine::TransitionTo, "[LDN:STATE] TransitionTo: new_state=%d\n", $x1
dprintf ams::mitm::ldn::LdnStateMachine::SignalStateChange, "[LDN:STATE] SignalStateChange: signaling event\n"

# State machine operations
dprintf ams::mitm::ldn::LdnStateMachine::GetState, "[LDN:STATE] GetState\n"
dprintf ams::mitm::ldn::LdnStateMachine::IsInState, "[LDN:STATE] IsInState\n"
dprintf ams::mitm::ldn::LdnStateMachine::IsInitialized, "[LDN:STATE] IsInitialized\n"
dprintf ams::mitm::ldn::LdnStateMachine::IsNetworkActive, "[LDN:STATE] IsNetworkActive\n"

# State transition entries
dprintf ams::mitm::ldn::LdnStateMachine::Initialize, "[LDN:STATE] Initialize → Initialized\n"
dprintf ams::mitm::ldn::LdnStateMachine::Finalize, "[LDN:STATE] Finalize → None\n"
dprintf ams::mitm::ldn::LdnStateMachine::OpenAccessPoint, "[LDN:STATE] OpenAccessPoint → AccessPoint\n"
dprintf ams::mitm::ldn::LdnStateMachine::CloseAccessPoint, "[LDN:STATE] CloseAccessPoint → Initialized\n"
dprintf ams::mitm::ldn::LdnStateMachine::CreateNetwork, "[LDN:STATE] CreateNetwork → AccessPointCreated\n"
dprintf ams::mitm::ldn::LdnStateMachine::DestroyNetwork, "[LDN:STATE] DestroyNetwork → AccessPoint\n"
dprintf ams::mitm::ldn::LdnStateMachine::OpenStation, "[LDN:STATE] OpenStation → Station\n"
dprintf ams::mitm::ldn::LdnStateMachine::CloseStation, "[LDN:STATE] CloseStation → Initialized\n"
dprintf ams::mitm::ldn::LdnStateMachine::Connect, "[LDN:STATE] Connect → StationConnected\n"
dprintf ams::mitm::ldn::LdnStateMachine::Disconnect, "[LDN:STATE] Disconnect → Station\n"
dprintf ams::mitm::ldn::LdnStateMachine::SetError, "[LDN:STATE] SetError → error state\n"

# State callback registration
dprintf ams::mitm::ldn::LdnStateMachine::SetStateCallback, "[LDN:STATE] SetStateCallback: callback=%p\n", $x0
dprintf ams::mitm::ldn::LdnStateMachine::GetStateChangeEventHandle, "[LDN:STATE] GetStateChangeEventHandle\n"

# Utility
dprintf ams::mitm::ldn::LdnStateMachine::StateToString, "[LDN:STATE] StateToString\n"
dprintf ams::mitm::ldn::LdnStateMachine::ResultToString, "[LDN:STATE] ResultToString\n"

# ==========================================
# SharedState — thread-safe global state
# (ams::mitm::ldn namespace)
# ==========================================

dprintf ams::mitm::ldn::SharedState::SharedState, "[LDN:STATE] SharedState: constructor\n"

# State access
dprintf ams::mitm::ldn::SharedState::SetLdnState, "[LDN:STATE] SetLdnState: state=%d\n", $x1
dprintf ams::mitm::ldn::SharedState::GetLdnState, "[LDN:STATE] GetLdnState\n"
dprintf ams::mitm::ldn::SharedState::SetSessionInfo, "[LDN:STATE] SetSessionInfo: node_count=%d max=%d local=%d is_host=%d\n", $x1, $x2, $x3, $x4
dprintf ams::mitm::ldn::SharedState::GetSessionInfo, "[LDN:STATE] GetSessionInfo\n"
dprintf ams::mitm::ldn::SharedState::GetSessionInfoStruct, "[LDN:STATE] GetSessionInfoStruct\n"

# Game tracking
dprintf ams::mitm::ldn::SharedState::SetGameActive, "[LDN:STATE] SetGameActive: active=%d pid=%lu\n", $x1, $x2
dprintf ams::mitm::ldn::SharedState::IsGameActive, "[LDN:STATE] IsGameActive\n"
dprintf ams::mitm::ldn::SharedState::GetActiveProcessId, "[LDN:STATE] GetActiveProcessId\n"

# Process and whitelist
dprintf ams::mitm::ldn::SharedState::SetLdnPid, "[LDN:STATE] SetLdnPid: pid=%lu\n", $x1
dprintf ams::mitm::ldn::SharedState::GetLdnPid, "[LDN:STATE] GetLdnPid\n"
dprintf ams::mitm::ldn::SharedState::IsLdnPid, "[LDN:STATE] IsLdnPid: pid=%lu\n", $x1
dprintf ams::mitm::ldn::SharedState::IsLdnGame, "[LDN:STATE] IsLdnGame: program_id=0x%lx\n", $x1
dprintf ams::mitm::ldn::SharedState::LoadLdnWhitelist, "[LDN:STATE] LoadLdnWhitelist\n"
dprintf ams::mitm::ldn::SharedState::GetWhitelistSize, "[LDN:STATE] GetWhitelistSize\n"

# RTT and reconnect
dprintf ams::mitm::ldn::SharedState::SetLastRtt, "[LDN:STATE] SetLastRtt: rtt=%u\n", $x1
dprintf ams::mitm::ldn::SharedState::GetLastRtt, "[LDN:STATE] GetLastRtt\n"
dprintf ams::mitm::ldn::SharedState::RequestReconnect, "[LDN:STATE] RequestReconnect\n"
dprintf ams::mitm::ldn::SharedState::ConsumeReconnectRequest, "[LDN:STATE] ConsumeReconnectRequest\n"

echo [LDN] Shared State & State Machine: 42 dprintf points\n