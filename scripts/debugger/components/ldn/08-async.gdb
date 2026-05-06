# =========================================
# LDN:ASYNC
# =========================================

echo [LDN] Loading async breakpoints...\n
# Namespace: ams::mitm::ldn
dprintf ams::mitm::ldn::ICommunicationService::ConnectToServer, "[LDN:ASYNC] ConnectToServer: entering\n"
dprintf ams::mitm::ldn::ICommunicationService::DisconnectFromServer, "[LDN:ASYNC] DisconnectFromServer: entering\n"
dprintf ams::mitm::ldn::ICommunicationService::IsServerConnected, "[LDN:ASYNC] IsServerConnected: queried\n"
dprintf ams::mitm::ldn::ICommunicationService::HandleServerPacket, "[LDN:ASYNC] HandleServerPacket: id=%u size=%zu\n", $x2, $x3
dprintf ams::mitm::ldn::ICommunicationService::WaitForResponse, "[LDN:ASYNC] WaitForResponse: expected_id=%u timeout_ms=%lu\n", $x1, $x2
dprintf ams::mitm::ldn::ICommunicationService::ReceiveThreadEntry, "[LDN:ASYNC] ReceiveThreadEntry: thread started arg=%p\n", $x0
dprintf ams::mitm::ldn::ICommunicationService::ReceiveThreadFunc, "[LDN:ASYNC] ReceiveThreadFunc: loop iteration\n"
dprintf ams::mitm::ldn::ICommunicationService::P2pConnectThreadEntry, "[LDN:ASYNC] P2pConnectThreadEntry: thread started arg=%p\n", $x0

echo [LDN] async: 8 dprintf points\n
