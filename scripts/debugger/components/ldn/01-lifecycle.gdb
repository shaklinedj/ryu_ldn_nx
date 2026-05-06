# =========================================
# LDN:LIFECYCLE
# =========================================

echo [LDN] Loading lifecycle breakpoints...\n
# Namespace: ams::mitm::ldn
dprintf ams::mitm::ldn::IClientProcessMonitor::IClientProcessMonitor, "[LDN:LIFECYCLE] ClientProcessMonitor created\n"
dprintf ams::mitm::ldn::IClientProcessMonitor::~IClientProcessMonitor, "[LDN:LIFECYCLE] ClientProcessMonitor destroyed\n"
dprintf ams::mitm::ldn::LdnConfigService::LdnConfigService, "[LDN:LIFECYCLE] ConfigService constructor\n"
dprintf ams::mitm::ldn::LdnConfigService::GetVersion, "[LDN:LIFECYCLE] Version queried\n"
dprintf ams::mitm::ldn::LdnConfigService::GetConnectionStatus, "[LDN:LIFECYCLE] Connection status queried\n"
dprintf ams::mitm::ldn::LdnConfigService::GetLdnState, "[LDN:LIFECYCLE] LDN state queried\n"
dprintf ams::mitm::ldn::LdnConfigService::GetSessionInfo, "[LDN:LIFECYCLE] Session info queried\n"
dprintf ams::mitm::ldn::ICommunicationService::ICommunicationService, "[LDN:LIFECYCLE] Communication service created\n"
dprintf ams::mitm::ldn::ICommunicationService::~ICommunicationService, "[LDN:LIFECYCLE] Communication service destroyed\n"
dprintf ams::mitm::ldn::LdnMitMService::LdnMitMService, "[LDN:LIFECYCLE] MitMService constructor\n"
dprintf ams::mitm::ldn::LdnMitMService::~LdnMitMService, "[LDN:LIFECYCLE] MitMService destructor\n"
dprintf ams::mitm::ldn::LdnMitMService::ShouldMitm, "[LDN:LIFECYCLE] ShouldMitm check\n"

echo [LDN] lifecycle: 12 dprintf points\n
