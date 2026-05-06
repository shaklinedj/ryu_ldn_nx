# =========================================
# NETWORK:STATE_MACHINE
# =========================================

echo [NETWORK] Loading state_machine breakpoints...\n
# Namespace: ryu_ldn::network
dprintf ryu_ldn::network::ConnectionStateMachine::ConnectionStateMachine, "[NETWORK:STATE_MACHINE] State transition\n"
dprintf ryu_ldn::network::ConnectionStateMachine::is_connected, "[NETWORK:STATE_MACHINE] is_connected\n"
dprintf ryu_ldn::network::ConnectionStateMachine::is_transitioning, "[NETWORK:STATE_MACHINE] is_transitioning\n"
dprintf ryu_ldn::network::ConnectionStateMachine::process_event, "[NETWORK:STATE_MACHINE] Processing event\n"
dprintf ryu_ldn::network::ConnectionStateMachine::set_state_change_callback, "[NETWORK:STATE_MACHINE] set_state_change_callback: cb=%p\n", $x0
dprintf ryu_ldn::network::ConnectionStateMachine::force_state, "[NETWORK:STATE_MACHINE] force_state: new=%d\n", $x1
dprintf ryu_ldn::network::ConnectionStateMachine::is_valid_transition, "[NETWORK:STATE_MACHINE] is_valid_transition\n"
dprintf ryu_ldn::network::ConnectionStateMachine::transition_to, "[NETWORK:STATE_MACHINE] transition_to: old=%d new=%d event=%d\n", $x1, $x2, $x3
dprintf ryu_ldn::network::ReconnectManager::get_config, "[NETWORK:STATE_MACHINE] ReconnectManager get_config\n"
dprintf ryu_ldn::network::ReconnectManager::set_config, "[NETWORK:STATE_MACHINE] ReconnectManager set_config\n"
dprintf ryu_ldn::network::Socket::Socket, "[NETWORK:STATE_MACHINE] Socket created\n"
dprintf ryu_ldn::network::Socket::~Socket, "[NETWORK:STATE_MACHINE] Socket destroyed\n"
dprintf ryu_ldn::network::Socket::connect, "[NETWORK:STATE_MACHINE] Socket connect\n"
dprintf ryu_ldn::network::Socket::close, "[NETWORK:STATE_MACHINE] Socket closed\n"
dprintf ryu_ldn::network::Socket::is_connected, "[NETWORK:STATE_MACHINE] Socket is_connected\n"
dprintf ryu_ldn::network::socket_init, "[NETWORK:STATE_MACHINE] socket_init\n"
dprintf ryu_ldn::network::socket_exit, "[NETWORK:STATE_MACHINE] socket_exit\n"

echo [NETWORK] state_machine: 17 dprintf points\n
