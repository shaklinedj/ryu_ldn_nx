# ==========================================
# Network Component - State Machine
# ==========================================
# ConnectionStateMachine transitions and state queries
# Using dprintf for automatic continue

echo [NETWORK] Loading state machine breakpoints...\n

# State transitions (also traced in 05-state-callbacks from RyuLdnClient side)
dprintf ryu_ldn::network::ConnectionStateMachine::transition_to, "[NETWORK:STATE] State transition\n"

# State queries
dprintf ryu_ldn::network::ConnectionStateMachine::is_connected, "[NETWORK:STATE] Connection state check\n"
dprintf ryu_ldn::network::ConnectionStateMachine::is_transitioning, "[NETWORK:STATE] Transition state check\n"
dprintf ryu_ldn::network::ConnectionStateMachine::is_valid_transition, "[NETWORK:STATE] Validating transition\n"

# State change callback
dprintf ryu_ldn::network::ConnectionStateMachine::set_state_change_callback, "[NETWORK:STATE] State callback set\n"
dprintf ryu_ldn::network::ConnectionStateMachine::force_state, "[NETWORK:STATE] Forcing state change\n"

# Process event
dprintf ryu_ldn::network::ConnectionStateMachine::process_event, "[NETWORK:STATE] Processing event\n"

# ReconnectManager state
dprintf ryu_ldn::network::ReconnectManager::get_config, "[NETWORK:STATE] ReconnectManager get_config\n"
dprintf ryu_ldn::network::ReconnectManager::set_config, "[NETWORK:STATE] ReconnectManager set_config\n"

# Socket lifecycle
dprintf ryu_ldn::network::Socket::Socket, "[NETWORK:STATE] Socket created\n"
dprintf ryu_ldn::network::Socket::~Socket, "[NETWORK:STATE] Socket destroyed\n"
dprintf ryu_ldn::network::Socket::connect, "[NETWORK:STATE] Socket connect\n"
dprintf ryu_ldn::network::Socket::close, "[NETWORK:STATE] Socket closed\n"
dprintf ryu_ldn::network::Socket::is_connected, "[NETWORK:STATE] Socket is_connected\n"

# Socket init/exit
dprintf ryu_ldn::network::socket_init, "[NETWORK:STATE] socket_init\n"
dprintf ryu_ldn::network::socket_exit, "[NETWORK:STATE] socket_exit\n"

echo [NETWORK] State machine: 16 dprintf points\n