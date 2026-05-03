# ==========================================
# Memory Trace - Watchpoints
# ==========================================
# Strategic memory watchpoints for critical data
# Updated for async architecture: receive thread, events, shared state

echo [MEMORY] Loading watchpoint system...\n

# Helper command to set watchpoints on important structures
define watch-client-state
    echo [MEMORY] Setting watchpoints on client state...\n
    echo [MEMORY] Use: watch *(int*)0x<address> to monitor state changes\n
    echo [MEMORY] Tip: Find address with: print &ryu_ldn::network::RyuLdnClient::m_state_machine\n
    echo [MEMORY] Example: watch *(int*)0x71000000\n
end

document watch-client-state
Set watchpoints on critical client state variables.
You must provide the runtime address (find it with 'print &varname').
end

# Define watchpoint for network state
define watch-network-state
    echo [MEMORY] Setting watchpoints on network state...\n
    echo [MEMORY] Use: watch *(int*)0x<address> to monitor network state changes\n
    echo [MEMORY] Key variables to watch:\n
    echo [MEMORY]   ams::mitm::ldn::LdnStateMachine::m_state\n
    echo [MEMORY]   ams::mitm::ldn::SharedState::m_ldn_state\n
    echo [MEMORY]   ryu_ldn::network::ConnectionStateMachine::m_state\n
end

document watch-network-state
Set watchpoints on network state variables.
Shows which variables to watch for state machine transitions.
end

# Monitor connection state
define watch-connection
    echo [MEMORY] Monitoring connection state changes...\n
    echo [MEMORY] Key breakpoints for connection state transitions:\n
    echo [MEMORY]   ams::mitm::ldn::ICommunicationService::ConnectToServer\n
    echo [MEMORY]   ams::mitm::ldn::ICommunicationService::DisconnectFromServer\n
    echo [MEMORY]   ams::mitm::ldn::ICommunicationService::HandleServerPacket\n
    echo [MEMORY]   ryu_ldn::network::ConnectionStateMachine::transition_to\n
    echo [MEMORY]   ryu_ldn::network::RyuLdnClient::try_connect\n
    echo [MEMORY]\n
    echo [MEMORY] For state callback observation:\n
    echo [MEMORY]   ryu_ldn::network::RyuLdnClient::update\n
    echo [MEMORY]   ryu_ldn::network::RyuLdnClient::process_handshake_response\n
    echo [MEMORY]\n
    echo [MEMORY] Use 'watch *(int*)<addr>' on m_state fields to catch changes\n
end

document watch-connection
Watch connection-related variables for changes.
Shows key breakpoints and variables for the async event-driven architecture.
end

# Watch the async architecture shared state (shared_mutex, events)
define watch-async-state
    echo [MEMORY] Setting watchpoints on async architecture state...\n
    echo [MEMORY] Key synchronization primitives to watch:\n
    echo [MEMORY]\n
    echo [MEMORY] Mutex (protects shared state between recv and IPC threads):\n
    echo [MEMORY]   ams::mitm::ldn::ICommunicationService::m_shared_mutex\n
    echo [MEMORY]\n
    echo [MEMORY] Events (signaled by recv thread, waited by IPC handlers):\n
    echo [MEMORY]   m_handshake_event  - signaled when RyuLdnClient reaches Ready\n
    echo [MEMORY]   m_response_event   - signaled on expected packet response\n
    echo [MEMORY]   m_scan_event       - signaled on ScanReplyEnd\n
    echo [MEMORY]   m_error_event      - signaled on NetworkError\n
    echo [MEMORY]   m_reject_event     - signaled on RejectReply\n
    echo [MEMORY]\n
    echo [MEMORY] Thread flag:\n
    echo [MEMORY]   m_recv_thread_running (atomic<bool>)\n
    echo [MEMORY]\n
    echo [MEMORY] Use: watch *(int*)&object.m_shared_mutex to find locks\n
    echo [MEMORY] Use: print &object to get address of ICommunicationService instance\n
end

document watch-async-state
Show watchpoints for async architecture synchronization primitives.
Covers m_shared_mutex, os::Event objects, and m_recv_thread_running.
end

# Watch LDN state machine transitions
define watch-ldn-state
    echo [MEMORY] Setting watchpoints on LDN state machine...\n
    echo [MEMORY] Key state transition points:\n
    echo [MEMORY]\n
    echo [MEMORY] LdnStateMachine (CommState):\n
    echo [MEMORY]   ams::mitm::ldn::LdnStateMachine::TransitionTo\n
    echo [MEMORY]   ams::mitm::ldn::LdnStateMachine::SignalStateChange\n
    echo [MEMORY]\n
    echo [MEMORY] ConnectionStateMachine (ConnectionState):\n
    echo [MEMORY]   ryu_ldn::network::ConnectionStateMachine::transition_to\n
    echo [MEMORY]\n
    echo [MEMORY] RyuLdnClient state callbacks:\n
    echo [MEMORY]   m_state_callback invocations in update(), try_connect(),\n
    echo [MEMORY]   disconnect(), process_handshake_response()\n
    echo [MEMORY]\n
    echo [MEMORY] Use: break ams::mitm::ldn::LdnStateMachine::TransitionTo\n
end

document watch-ldn-state
Show watchpoints and breakpoints for LDN state machine transitions.
Covers both LdnStateMachine (CommState) and ConnectionStateMachine.
end

# Helper for setting conditional watchpoints
define watch-cond
    if $argc != 2
        echo Usage: watch-cond <address> <condition>\n
        echo Example: watch-cond 0x12345678 "value > 100"\n
    else
        eval "watch *(%s)" $arg0
        eval "condition $bpnum %s" $arg1
        echo Conditional watchpoint set\n
    end
end

document watch-cond
Set a conditional watchpoint
Usage: watch-cond <address> <condition>
end

echo [MEMORY] Watchpoint commands loaded\n
echo [MEMORY] Commands: watch-client-state, watch-network-state, watch-connection,\n
echo [MEMORY]   watch-async-state, watch-ldn-state, watch-cond\n