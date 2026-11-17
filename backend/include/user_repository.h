
#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include "user.h"
#include <mysql.h>
#include <windows.h>
#include <bcrypt.h>

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <stdexcept>

class UserRepository {
private:

    MYSQL* connection;
    std::mutex& databaseMutex;

    static constexpr ULONG SALT_SIZE = 16;
    static constexpr ULONG HASH_SIZE = 32;
    static constexpr ULONGLONG ITERATIONS = 100000;

    // Convert binary data into hexadecimal text.
    std::string toHex(const std::vector<unsigned char>& data) const {
        static const char digits[] = "0123456789abcdef";

        std::string result;

        for (unsigned char byte : data) {
            result += digits[byte >> 4];
            result += digits[byte & 0x0F];
        }

        return result;
    }

    // Convert hexadecimal text back into binary data.
    bool fromHex(
        const std::string& input,
        std::vector<unsigned char>& output
    ) const {

        if (input.size() % 2 != 0)
            return false;

        output.clear();

        for (size_t i = 0; i < input.size(); i += 2) {
            try {
                unsigned int value =
                    std::stoul(input.substr(i, 2), nullptr, 16);

                output.push_back(
                    static_cast<unsigned char>(value)
                );
            }
            catch (...) {
                return false;
            }
        }

        return true;
    }

    // Escape user input before using it in an SQL query.
    std::string escapeSQL(const std::string& value) const {

        std::string escaped(value.size() * 2 + 1, '\0');

        unsigned long length = mysql_real_escape_string(
            connection,
            &escaped[0],
            value.c_str(),
            static_cast<unsigned long>(value.size())
        );

        escaped.resize(length);

        return escaped;
    }

    bool hashPassword(
        const std::string& password,
        const std::vector<unsigned char>& salt,
        std::vector<unsigned char>& hash
    ) const {

        BCRYPT_ALG_HANDLE algorithm = nullptr;

        NTSTATUS status = BCryptOpenAlgorithmProvider(
            &algorithm,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            BCRYPT_ALG_HANDLE_HMAC_FLAG
        );

        if (status < 0)
            return false;

        hash.resize(HASH_SIZE);

        status = BCryptDeriveKeyPBKDF2(
            algorithm,
            reinterpret_cast<PUCHAR>(
                const_cast<char*>(password.data())
            ),
            static_cast<ULONG>(password.size()),
            const_cast<PUCHAR>(salt.data()),
            static_cast<ULONG>(salt.size()),
            ITERATIONS,
            hash.data(),
            HASH_SIZE,
            0
        );

        BCryptCloseAlgorithmProvider(algorithm, 0);

        return status >= 0;
    }

    bool createPasswordHash(
        const std::string& password,
        std::string& storedHash
    ) const {

        std::vector<unsigned char> salt(SALT_SIZE);
        std::vector<unsigned char> hash;

        NTSTATUS status = BCryptGenRandom(
            nullptr,
            salt.data(),
            SALT_SIZE,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );

        if (status < 0)
            return false;

        if (!hashPassword(password, salt, hash))
            return false;

        // Store salt and hash together in password_hash.
        storedHash = toHex(salt) + ":" + toHex(hash);

        return true;
    }

    bool verifyPassword(
        const std::string& password,
        const std::string& storedHash
    ) const {

        size_t separator = storedHash.find(':');

        if (separator == std::string::npos)
            return false;

        std::vector<unsigned char> salt;
        std::vector<unsigned char> originalHash;
        std::vector<unsigned char> calculatedHash;

        if (!fromHex(storedHash.substr(0, separator), salt))
            return false;

        if (!fromHex(storedHash.substr(separator + 1), originalHash))
            return false;

        if (salt.size() != SALT_SIZE ||
            originalHash.size() != HASH_SIZE) {
            return false;
        }

        if (!hashPassword(password, salt, calculatedHash))
            return false;

        unsigned char difference = 0;

        for (size_t i = 0; i < HASH_SIZE; i++) {
            difference |= calculatedHash[i] ^ originalHash[i];
        }

        return difference == 0;
    }

public:

    explicit UserRepository(
        MYSQL* dbConnection,
        std::mutex& sharedMutex
    )
        : connection(dbConnection),
        databaseMutex(sharedMutex) {

        if (connection == nullptr) {
            throw std::runtime_error(
                "UserRepository received an invalid database connection."
            );
        }
    }

    // Register a passenger.
    bool registerUser(
        const std::string& username,
        const std::string& password,
        bool premium = false
    ) {

        if (username.empty() || password.size() < 8)
            return false;

        std::lock_guard<std::mutex> lock(databaseMutex);

        std::string storedHash;

        if (!createPasswordHash(password, storedHash))
            return false;

        std::string safeUsername = escapeSQL(username);
        std::string safeHash = escapeSQL(storedHash);

        std::string query =
            "INSERT INTO users "
            "(username, password_hash, role, is_premium) VALUES ('" +
            safeUsername + "', '" +
            safeHash + "', 'PASSENGER', " +
            (premium ? "TRUE" : "FALSE") + ")";

        if (mysql_query(connection, query.c_str()) != 0) {
            return false;
        }

        return mysql_affected_rows(connection) == 1;
    }

    // Authenticate a passenger.
    bool authenticate(
        const std::string& username,
        const std::string& password
    ) const {

        std::lock_guard<std::mutex> lock(databaseMutex);

        std::string safeUsername = escapeSQL(username);

        std::string query =
            "SELECT password_hash FROM users WHERE username = '" +
            safeUsername + "' LIMIT 1";

        if (mysql_query(connection, query.c_str()) != 0)
            return false;

        MYSQL_RES* result = mysql_store_result(connection);

        if (result == nullptr)
            return false;

        MYSQL_ROW row = mysql_fetch_row(result);

        bool authenticated = false;

        if (row != nullptr && row[0] != nullptr) {
            authenticated = verifyPassword(password, row[0]);
        }

        mysql_free_result(result);

        return authenticated;
    }

    // Get passenger role.
    std::string getRole(
        const std::string& username
    ) const {

        std::lock_guard<std::mutex> lock(databaseMutex);

        std::string safeUsername = escapeSQL(username);

        std::string query =
            "SELECT role, is_premium FROM users "
            "WHERE username = '" + safeUsername + "' LIMIT 1";

        if (mysql_query(connection, query.c_str()) != 0)
            return "";

        MYSQL_RES* result = mysql_store_result(connection);

        if (result == nullptr)
            return "";

        MYSQL_ROW row = mysql_fetch_row(result);

        std::string role = "";

        if (row != nullptr && row[0] != nullptr) {

            std::string databaseRole = row[0];

            if (databaseRole == "ADMIN") {
                role = "Admin";
            }
            else if (row[1] != nullptr &&
                     std::string(row[1]) == "1") {
                role = "Premium Passenger";
            }
            else {
                role = "Passenger";
            }
        }

        mysql_free_result(result);

        return role;
    }

    // Change passenger password.
    bool changePassword(
        const std::string& username,
        const std::string& currentPassword,
        const std::string& newPassword
    ) {

        if (newPassword.size() < 8)
            return false;

        std::lock_guard<std::mutex> lock(databaseMutex);

        std::string safeUsername = escapeSQL(username);

        std::string selectQuery =
            "SELECT password_hash FROM users WHERE username = '" +
            safeUsername + "' LIMIT 1";

        if (mysql_query(connection, selectQuery.c_str()) != 0)
            return false;

        MYSQL_RES* result = mysql_store_result(connection);

        if (result == nullptr)
            return false;

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == nullptr || row[0] == nullptr) {
            mysql_free_result(result);
            return false;
        }

        std::string oldHash = row[0];

        mysql_free_result(result);

        if (!verifyPassword(currentPassword, oldHash))
            return false;

        std::string newHash;

        if (!createPasswordHash(newPassword, newHash))
            return false;

        std::string safeNewHash = escapeSQL(newHash);

        std::string updateQuery =
            "UPDATE users SET password_hash = '" +
            safeNewHash + "' WHERE username = '" +
            safeUsername + "'";

        if (mysql_query(connection, updateQuery.c_str()) != 0)
            return false;

        return mysql_affected_rows(connection) == 1;
    }

    // Get all passenger usernames for admin dashboard.
    std::vector<std::string> getAllUsernames() const {

        std::lock_guard<std::mutex> lock(databaseMutex);

        std::vector<std::string> usernames;

        const char* query =
            "SELECT username FROM users ORDER BY user_id";

        if (mysql_query(connection, query) != 0)
            return usernames;

        MYSQL_RES* result = mysql_store_result(connection);

        if (result == nullptr)
            return usernames;

        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result)) != nullptr) {
            if (row[0] != nullptr) {
                usernames.push_back(row[0]);
            }
        }

        mysql_free_result(result);

        return usernames;
    }
};

#endif
