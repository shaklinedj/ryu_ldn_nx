# =========================================
# DEBUG:LOGGER
# =========================================

echo [DEBUG] Loading logger breakpoints...\n
# Namespace: ryu_ldn::debug
dprintf ryu_ldn::debug::Logger::init, "[DEBUG:LOGGER] Logger initialized\n"
dprintf ryu_ldn::debug::Logger::open_file, "[DEBUG:LOGGER] Log file opened\n"
dprintf ryu_ldn::debug::Logger::close_file, "[DEBUG:LOGGER] Log file closed\n"

echo [DEBUG] logger: 3 dprintf points\n
