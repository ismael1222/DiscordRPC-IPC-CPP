#pragma once
#include <string>
#include <optional>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct Timestamps {
    std::optional<int> start;
    std::optional<int> end;
};
inline void to_json(nlohmann::json& j, const Timestamps& t) {
    j = nlohmann::json{};
    if (t.start) {
        j["start"] = t.start.value();
    }
    if (t.end) {
        j["end"] = t.end.value();
    }
}

struct Emoji {
    std::optional<std::string> name;
    std::optional<std::string> id; //snowflake
};
inline void to_json(nlohmann::json& j, const Emoji& e) {
    j = nlohmann::json{};
    if (e.name) {
        j["name"] = e.name.value();
    }
    if (e.id) {
        j["id"] = e.id.value();
    }
}
struct Assets {
    std::optional<std::string> large_image;
    std::optional<std::string> large_text;
    std::optional<std::string> small_image;
    std::optional<std::string> small_text;
};
inline void to_json(nlohmann::json& j, const Assets& a) {
    j = nlohmann::json{};
    if (a.large_image) {
        j["large_image"] = a.large_image.value();
    }
    if (a.large_text) {
        j["large_text"] = a.large_text.value();
    }
    if (a.small_image) {
        j["small_image"] = a.small_image.value();
    }
    if (a.small_text) {
        j["small_text"] = a.small_text.value();
    }
}
struct Party {
    std::optional<std::string> id;
    std::optional<int> size;
    std::optional<int> max;
};
inline void to_json(nlohmann::json& j, const Party& p) {
    if(p.id) {
        j["id"] = p.id.value();
    }
    if(p.size) {
        j["size"] = p.size.value();
    }
    if(p.max) {
        j["max"] = p.max.value();
    }
}
struct Button {
    std::string label;
    std::string url;
};
inline void to_json(nlohmann::json& j, const Button& b) {
    j["label"] = b.label;
    j["url"] = b.url;
}
struct Secrets {
    std::optional<std::string> join;
    std::optional<std::string> spectate;
    std::optional<std::string> match;
};
inline void to_json(nlohmann::json& j, const Secrets& s) {
    if(s.join) {
        j["join"] = s.join.value();
    }
    if(s.spectate) {
        j["spectate"] = s.spectate.value();
    }
    if(s.match) {
        j["match"] = s.match.value();
    }
}
struct Activity {
    std::string name = "";
    int type = -1;
    std::optional<std::string> url;
    //nullable
    int created_at = -1;
    std::optional<Timestamps> timestamps;
    std::optional<std::string> application_id;
    //snowflake, not string.
    //application_id might just be the appID required for the client
    std::optional<int> status_display_type;
    //nullable
    std::optional<std::string> details;
    //nullable
    std::optional<std::string> details_url;
    //nullable
    std::optional<std::string> state;
    //nullable
    std::optional<std::string> state_url;
    //nullable
    std::optional<Emoji> emoji;
    //nullable
    std::optional<Party> party;
    std::optional<Assets> assets;
    std::optional<Secrets> secrets;
    std::optional<bool> instance;
    std::optional<int> flags;
    std::optional<std::vector<Button>> buttons;
};
struct DiscordRPCConfig {
    int intervalMs = 15*1000;     
    bool retry = true;           
    int retryDelayMs = 5*1000; 
};
class DiscordRPC {
public:
    explicit DiscordRPC(const std::string& cid);
    ~DiscordRPC() = default;
    // Callbacks
    std::function<void()> onConnect;
    std::function<void()> onDisconnect;
    std::function<void()> onHandshake;
    std::function<void()> onActivitySet;
    std::function<void(const std::string&)> onError;
    void run (int intervalMs, bool retry = true, int retryDelayMs = 5000);    //blocking
    void start(int intervalMs, bool retry = true, int retryDelayMs = 5000);  //non-blocking
    void stop ();
    void setActivity(const Activity& act);
    // Config setters
    void setInterval(int ms) {
        config.intervalMs = ms;
    }
    void setRetry(bool r) {
        config.retry = r;
    }
    void setRetryDelay(int ms) {
        config.retryDelayMs = ms;
    }
private:
    DiscordRPCConfig config;
    std::string client_id;
    bool nonstop = true;
    Activity activity;
    std::mutex activityMutex;
    int sock = -1;
    json buildActivityPayload();
    bool sendJSON(int sock, int op, const json& j);
    json readJSON(int sock);
    int connectDiscord(const char* path);
};
