
#pragma once

#include <mysql.h>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <mutex>

class Database {
private:
    MYSQL* connection;
    std::mutex databaseMutex;

public:
    Database() {
        connection = mysql_init(nullptr);

        if (connection == nullptr) {
            throw std::runtime_error("MySQL initialization failed");
        }

        const char* host = "127.0.0.1";
        const char* user = std::getenv("DB_USER");
        const char* password = std::getenv("DB_PASSWORD");
        const char* database = "railway_reservation";

        if (user == nullptr || password == nullptr) {
            mysql_close(connection);
            connection = nullptr;
            throw std::runtime_error("Database credentials are missing");
        }

        if (mysql_real_connect(
                connection,
                host,
                user,
                password,
                database,
                3306,
                nullptr,
                0
            ) == nullptr) {

            std::string error = mysql_error(connection);
            mysql_close(connection);
            connection = nullptr;

            throw std::runtime_error("MySQL connection failed: " + error);
        }

        std::cout << "Connected to Railway MySQL database successfully!"
                  << std::endl;
    }

    ~Database() {
        if (connection != nullptr) {
            mysql_close(connection);
        }
    }

    MYSQL* getConnection() const {
        return connection;
    }

    std::mutex& getMutex() {
        return databaseMutex;
    }

    bool isConnected() const {
        return connection != nullptr;
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
};
