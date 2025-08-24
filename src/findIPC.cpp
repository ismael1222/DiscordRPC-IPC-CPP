#include "findIPC.h"
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <sstream>
#include <iostream>

std::string findIPC::find() {
    std::array<char, 128> buffer;
    std::string result;
    // popen abre un pipe y ejecuta el comando
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("find / -type s -name 'discord-ipc-*' 2>/dev/null", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("Failed to open pipe");
    }
    // Leer la salida
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}
