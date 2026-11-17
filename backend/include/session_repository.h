
#ifndef SESSION_REPOSITORY_H
#define SESSION_REPOSITORY_H

#include <windows.h>
#include <bcrypt.h>

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <chrono>

struct SessionInfo {
    std::string username;
    std::string role;
};

class SessionRepository {
private:
    struct SessionRecord {
        SessionInfo info;
        std::chrono::steady_clock::time_point expiresAt;
    };

    std::unordered_map<std::string, SessionRecord> sessions;
    mutable std::mutex sessionMutex;

    static constexpr ULONG TOKEN_SIZE = 32;
    static constexpr int SESSION_DURATION_HOURS = 2;

    std::string generateToken() const {
        std::vector<unsigned char> bytes(TOKEN_SIZE);

        NTSTATUS status = BCryptGenRandom(
            nullptr,
            bytes.data(),
            TOKEN_SIZE,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );

        if (status < 0) {
            return "";
        }

        const char hex[] = "0123456789abcdef";
        std::string token;

        for (unsigned char byte : bytes) {
            token += hex[byte >> 4];
            token += hex[byte & 0x0F];
        }

        return token;
    }

public:
    // Create a new session after successful login.
    std::string createSession(
        const std::string& username,
        const std::string& role
    ) {
        std::string token = generateToken();

        if (token.empty()) {
            return "";
        }

        SessionRecord record;
        record.info.username = username;
        record.info.role = role;
        record.expiresAt =
            std::chrono::steady_clock::now() +
            std::chrono::hours(SESSION_DURATION_HOURS);

        std::lock_guard<std::mutex> lock(sessionMutex);
        sessions[token] = record;

        return token;
    }

    // Validate a token and retrieve its user information.
    bool getSession(
        const std::string& token,
        SessionInfo& info
    ) {
        std::lock_guard<std::mutex> lock(sessionMutex);

        auto it = sessions.find(token);

        if (it == sessions.end()) {
            return false;
        }

        if (std::chrono::steady_clock::now() >= it->second.expiresAt) {
            sessions.erase(it);
            return false;
        }

        info = it->second.info;
        return true;
    }

    // Delete a session during logout.
    bool deleteSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessionMutex);
        return sessions.erase(token) > 0;
    }
};

#endif
