#include "DiscordRPC.hpp"
#include "findIPC.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
//colors
#define ORANGE  "\033[38;2;255;180;0m"
// ------------------ constructor / destructor ------------------
DiscordRPC::DiscordRPC(const std::string& cid) 
    : client_id(cid), nonstop(true) {}
// ------------------ stop rpc loop ------------------
void DiscordRPC::stop() {
    nonstop = false;
}
// ------------------ set activity ------------------
void DiscordRPC::setActivity(const Activity& act) {
    std::lock_guard<std::mutex> lock(activityMutex);
    activity = act;
}
// ------------------ timestamp helper ------------------
int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
//if non-defined values are passed.
void validateActivity(const Activity& act) {
    if (act.name.empty()) {
        throw std::invalid_argument("Invalid activity: 'name' is required");
    }
    if (act.type < 0 || act.type > 4) {
        throw std::invalid_argument("Invalid activity: 'type' is required to be between 0 and 4 (inclusive)");
    }
    if (act.created_at < 0) {
        throw std::invalid_argument("Invalid activity: 'created_at' is negative");
    }
}


// ------------------ build activity payload ------------------
json DiscordRPC::buildActivityPayload() {
    std::lock_guard<std::mutex> lock(activityMutex);
    json j;
    j["name"] = activity.name;
    j["type"] = activity.type;
    if(activity.url) {
        j["url"] = activity.url.value();
    }
    j["created_at"] = activity.created_at;
    if(activity.timestamps) {
        j["timestamps"] = activity.timestamps.value();
    }
    if(activity.status_display_type) {
        j["status_display_type"] = activity.status_display_type.value();
    }
    if(activity.details) {
        j["details"] = activity.details.value();
    }
    if(activity.details_url) {
        j["details_url"] = activity.details_url.value();
    }

    if(activity.state) {
        j["state"] = activity.state.value();
    }
    if(activity.state_url) {
        j["state_url"] = activity.state_url.value();
    }
    if(activity.emoji) {
        j["emoji"] = activity.emoji.value();
    }
    if(activity.party) {
        j["party"] = activity.party.value();
    }
    if(activity.assets) {
        j["assets"] = activity.assets.value();
    }
    if(activity.secrets) {
        j["secrets"] = activity.secrets.value();
    }
    if(activity.instance) {
        j["instance"] = activity.instance.value();
    }
    if(activity.flags) {
        j["flags"] = activity.flags;
    }
    if(activity.buttons) {
        j["buttons"] = activity.buttons.value();
    }
    return j; // <-- ALWAYS return at the end
}
// ------------------ send json ------------------
bool DiscordRPC::sendJSON(int sock, int op, const json& j) {
    std::string payload = j.dump();
    struct {
        int32_t op;
        int32_t length;
    } header{op, static_cast<int32_t>(payload.size())};
    if(write(sock, &header, sizeof(header)) != sizeof(header)) {
        return false;
    }
    if(write(sock, payload.c_str(), payload.size()) != (ssize_t)payload.size()) {
        return false;
    }
    return true;
}
// ------------------ read json ------------------
json DiscordRPC::readJSON(int sock) {
    struct {
        int32_t op;
        int32_t length;
    } header{};
    if(read(sock, &header, sizeof(header)) != sizeof(header)) {
        return nullptr;
    }
    std::vector<char> buffer(header.length);
    if(read(sock, buffer.data(), header.length) != header.length) {
        return nullptr;
    }
    try {
        return json::parse(buffer);
    } catch(...) {
        return nullptr;
    }
}
// ------------------ connect ------------------
int DiscordRPC::connectDiscord(const char* path) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        std::perror("socket");
        return -1;
    }
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sock);
        return -1;
    }
    return sock;
}
// ------------------ main loop ------------------
void DiscordRPC::run(int intervalMs, bool retry, int retryDelayMs) {
    validateActivity(activity);
//too-low values warnings
    if (intervalMs <= 4000) {
        std::cerr << ORANGE << "DiscordRPC warning:"<< RESETSTYLE <<" intervalMs <= 4s may trigger a rate limit (Discord allows 5 queries per 20s)." << std::endl;
    }
    std::string socketPathStr = findIPC::find();
    if (socketPathStr.empty()) {
        if (onError) {
            try {
                onError("No Discord socket found");
            } catch (...) {}
        }
        return;
    }
    while (nonstop) {
        // ===== 1) Connect with retry if failed =====
        while (nonstop) {
            sock = connectDiscord(socketPathStr.c_str());
            if (sock == -1) {
                if (onError) {
                    if(!retry) {
                        nonstop = false;
                        continue; //stop if no-retry
                    } else {
                        try {
                            onError("Couldn't connect to Discord. Retrying in "+std::to_string(retryDelayMs) +"ms...");
                        } catch (...) {}
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                    continue;
                }
            }
            if (onConnect) {
                try {
                    onConnect();
                } catch (...) {
                    std::cerr << "Exception in onConnect\n";
                }
            }
            break;
        }
        if (!nonstop) {
            break;    // salir si se pidió stop
        }
        // ===== 2) Handshake =====
        json handshake = {{"v", 1}, {"client_id", client_id}};
        if (!sendJSON(sock, 0, handshake)) {
            if (onError) {
                try {
                    onError("Error sending handshake");
                } catch (...) {}
            }
            close(sock);
            if (onDisconnect) {
                try {
                    onDisconnect();
                } catch (...) {}
            }
            continue;
        }
        json response = readJSON(sock);
        if (!response.contains("evt") || response["evt"] != "READY") {
            if (onError) {
                try {
                    onError("Handshake failed");
                } catch (...) {}
            }
            close(sock);
            if (onDisconnect) {
                try {
                    onDisconnect();
                } catch (...) {}
            }
            continue; // reintentar
        }
        if (onHandshake) {
            try {
                onHandshake();
            } catch (...) {
                std::cerr << "Exception in onHandshake\n";
            }
        }
        // ===== 3) Loop principal de actividad =====
        while (nonstop) {
            json activityPayload = {
                {"cmd", "SET_ACTIVITY"},
                {"args", {{"pid", (int)getpid()}, {"activity", buildActivityPayload()}}},
                {"nonce", std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())}
            };
            if (!sendJSON(sock, 1, activityPayload)) {
                if (onError) {
                    try {
                        onError("Error sending activity");
                    } catch (...) {}
                }
                break; // salir y reconectar
            }
            if (onActivitySet) {
                try {
                    onActivitySet();
                } catch (...) {
                    std::cerr << "Exception in onActivitySet\n";
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        }
        // ===== 4) Desconexión =====
        close(sock);
        if (onDisconnect) {
            try {
                onDisconnect();
            } catch (...) {
                std::cerr << "Exception in onDisconnect\n";
            }
        }
        std::cerr << "Socket cerrado, intentando reconectar...\n";
    }
}
// ------------------ run in background ------------------
void DiscordRPC::start(int intervalMs, bool retry, int retryDelayMs) {
    std::thread([this, intervalMs, retry, retryDelayMs]() {
        this->run(intervalMs, retry, retryDelayMs);
    }).detach();
}
