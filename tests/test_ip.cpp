#include <iostream>
#include <iomanip>
#include <cstdint>

int main() {
    uint32_t ip = 0x0A720001; // 10.114.0.1 in Ryujinx format (Big-Endian read as uint32)
    std::cout << "IP: 0x" << std::hex << ip << std::endl;
    std::cout << "First octet: " << std::dec << ((ip >> 24) & 0xFF) << std::endl;
    std::cout << "Second octet: " << std::dec << ((ip >> 16) & 0xFF) << std::endl;
    std::cout << "Third octet: " << std::dec << ((ip >> 8) & 0xFF) << std::endl;
    std::cout << "Fourth octet: " << std::dec << (ip & 0xFF) << std::endl;

    uint32_t bcast = 0x0A72FFFF; // 10.114.255.255
    std::cout << "Broadcast last octet: " << std::dec << (bcast & 0xFF) << std::endl;
    
    // Test multicast 224.0.0.0
    uint32_t mcast = 0xE0000000;
    std::cout << "Is multicast: " << ((mcast & 0xF0000000) == 0xE0000000) << std::endl;
    return 0;
}
