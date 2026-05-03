# ==========================================
# LDN Component - Service Lifecycle
# ==========================================
# LDN service initialization and MITM service registration
# Using dprintf for automatic continue

echo [LDN] Loading service lifecycle breakpoints...\n

# Config service lifecycle
dprintf ams::mitm::ldn::LdnConfigService::LdnConfigService, "[LDN:LIFECYCLE] ConfigService constructor\n"
dprintf ams::mitm::ldn::LdnConfigService::GetVersion, "[LDN:LIFECYCLE] Version queried\n"
dprintf ams::mitm::ldn::LdnConfigService::GetConnectionStatus, "[LDN:LIFECYCLE] Connection status queried\n"
dprintf ams::mitm::ldn::LdnConfigService::GetLdnState, "[LDN:LIFECYCLE] LDN state queried\n"
dprintf ams::mitm::ldn::LdnConfigService::GetSessionInfo, "[LDN:LIFECYCLE] Session info queried\n"

# MITM service lifecycle (the service that intercepts ldn:u)
dprintf ams::mitm::ldn::LdnMitMService::LdnMitMService, "[LDN:LIFECYCLE] MitMService constructor\n"
dprintf ams::mitm::ldn::LdnMitMService::~LdnMitMService, "[LDN:LIFECYCLE] MitMService destructor\n"
dprintf ams::mitm::ldn::LdnMitMService::ShouldMitm, "[LDN:LIFECYCLE] ShouldMitm check\n"

# ICommunication service lifecycle
dprintf ams::mitm::ldn::ICommunicationService::ICommunicationService, "[LDN:LIFECYCLE] Communication service created\n"
dprintf ams::mitm::ldn::ICommunicationService::~ICommunicationService, "[LDN:LIFECYCLE] Communication service destroyed\n"

# Client process monitor
dprintf ams::mitm::ldn::IClientProcessMonitor::IClientProcessMonitor, "[LDN:LIFECYCLE] ClientProcessMonitor created\n"
dprintf ams::mitm::ldn::IClientProcessMonitor::~IClientProcessMonitor, "[LDN:LIFECYCLE] ClientProcessMonitor destroyed\n"

echo [LDN] Service lifecycle: 12 dprintf points\n