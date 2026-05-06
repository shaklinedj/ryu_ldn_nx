# =========================================
# LDN:OPS
# =========================================

echo [LDN] Loading ops breakpoints...\n
# Namespace: ams::mitm::ldn
dprintf ams::mitm::ldn::LdnConfigService::ForceReconnect, "[LDN:OPS] ForceReconnect\n"
dprintf ams::mitm::ldn::LdnConfigService::GetLastRtt, "[LDN:OPS] GetLastRtt\n"
dprintf ams::mitm::ldn::ICommunicationService::GetState, "[LDN:OPS] GetState\n"
dprintf ams::mitm::ldn::ICommunicationService::GetNetworkInfo, "[LDN:OPS] GetNetworkInfo\n"
dprintf ams::mitm::ldn::ICommunicationService::GetIpv4Address, "[LDN:OPS] GetIpv4Address\n"
dprintf ams::mitm::ldn::ICommunicationService::GetDisconnectReason, "[LDN:OPS] GetDisconnectReason\n"
dprintf ams::mitm::ldn::ICommunicationService::GetSecurityParameter, "[LDN:OPS] GetSecurityParameter\n"
dprintf ams::mitm::ldn::ICommunicationService::GetNetworkConfig, "[LDN:OPS] GetNetworkConfig\n"
dprintf ams::mitm::ldn::ICommunicationService::AttachStateChangeEvent, "[LDN:OPS] AttachStateChangeEvent\n"
dprintf ams::mitm::ldn::ICommunicationService::GetNetworkInfoLatestUpdate, "[LDN:OPS] GetNetworkInfoLatestUpdate\n"
dprintf ams::mitm::ldn::ICommunicationService::Scan, "[LDN:OPS] Scan\n"
dprintf ams::mitm::ldn::ICommunicationService::OpenAccessPoint, "[LDN:OPS] OpenAccessPoint\n"
dprintf ams::mitm::ldn::ICommunicationService::CloseAccessPoint, "[LDN:OPS] CloseAccessPoint\n"
dprintf ams::mitm::ldn::ICommunicationService::CreateNetwork, "[LDN:OPS] CreateNetwork\n"
dprintf ams::mitm::ldn::ICommunicationService::DestroyNetwork, "[LDN:OPS] DestroyNetwork\n"
dprintf ams::mitm::ldn::ICommunicationService::SetAdvertiseData, "[LDN:OPS] SetAdvertiseData\n"
dprintf ams::mitm::ldn::ICommunicationService::SetStationAcceptPolicy, "[LDN:OPS] SetStationAcceptPolicy\n"
dprintf ams::mitm::ldn::ICommunicationService::OpenStation, "[LDN:OPS] OpenStation\n"
dprintf ams::mitm::ldn::ICommunicationService::CloseStation, "[LDN:OPS] CloseStation\n"
dprintf ams::mitm::ldn::ICommunicationService::Connect, "[LDN:OPS] Connect\n"
dprintf ams::mitm::ldn::ICommunicationService::Disconnect, "[LDN:OPS] Disconnect\n"
dprintf ams::mitm::ldn::ICommunicationService::Initialize, "[LDN:OPS] Initialize\n"
dprintf ams::mitm::ldn::ICommunicationService::Finalize, "[LDN:OPS] Finalize\n"
dprintf ams::mitm::ldn::ICommunicationService::InitializeSystem2, "[LDN:OPS] InitializeSystem2\n"
dprintf ams::mitm::ldn::ICommunicationService::ScanPrivate, "[LDN:OPS] ScanPrivate\n"
dprintf ams::mitm::ldn::ICommunicationService::CreateNetworkPrivate, "[LDN:OPS] CreateNetworkPrivate\n"
dprintf ams::mitm::ldn::ICommunicationService::ConnectPrivate, "[LDN:OPS] ConnectPrivate\n"
dprintf ams::mitm::ldn::ICommunicationService::SetWirelessControllerRestriction, "[LDN:OPS] SetWirelessControllerRestriction\n"
dprintf ams::mitm::ldn::ICommunicationService::Reject, "[LDN:OPS] Reject: nodeId=%u\n", $x1
dprintf ams::mitm::ldn::ICommunicationService::AddAcceptFilterEntry, "[LDN:OPS] AddAcceptFilterEntry\n"
dprintf ams::mitm::ldn::ICommunicationService::ClearAcceptFilter, "[LDN:OPS] ClearAcceptFilter\n"
dprintf ams::mitm::ldn::ICommunicationService::SendProxyDataToServer, "[LDN:OPS] SendProxyDataToServer\n"

echo [LDN] ops: 32 dprintf points\n
