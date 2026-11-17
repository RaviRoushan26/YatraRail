
#ifndef TRAIN_REPOSITORY_H
#define TRAIN_REPOSITORY_H

#include "train.h"

#include <mysql.h>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <optional>


class TrainRepository {
private:
    MYSQL* connection;
    std::mutex& databaseMutex;
    mutable std::vector<Train> trainCache;

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

    std::string normalizeStation(std::string station) const {
        std::transform(
            station.begin(), station.end(), station.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            }
        );

        if (station == "mysuru") return "mysore";
        if (station == "bengaluru") return "bangalore";

        return station;
    }

    bool validClass(const std::string& type) const {
        return type == "AC" ||
            type == "Sleeper" ||
            type == "SLEEPER";
    }

    int capacity(const std::string& type) const {
        if (type == "AC")
            return 40;

        if (type == "Sleeper" || type == "SLEEPER")
            return 100;

        return 0;
    }

    bool trainExists(int number) const {
        std::string query =
            "SELECT train_id FROM trains WHERE train_number = " +
            std::to_string(number) + " LIMIT 1";

        if (mysql_query(connection, query.c_str()) != 0)
            return false;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return false;

        bool found = mysql_num_rows(result) > 0;
        mysql_free_result(result);

        return found;
    }

    int getTrainId(int number) const {
        std::string query =
            "SELECT train_id FROM trains WHERE train_number = " +
            std::to_string(number) + " LIMIT 1";

        if (mysql_query(connection, query.c_str()) != 0)
            return -1;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return -1;

        MYSQL_ROW row = mysql_fetch_row(result);

        int id = -1;
        if (row && row[0])
            id = std::stoi(row[0]);

        mysql_free_result(result);
        return id;
    }
    // Creates inventory rows the first time a date/class is used.
    bool initializeInventory(
        int trainId,
        const std::string& date,
        const std::string& type
    ) const {
        if (!validClass(type) || date.empty())
            return false;

        int total = capacity(type);

        std::string dbClassType =
            (type == "AC") ? "AC" : "SLEEPER";

        for (int seat = 1; seat <= total; ++seat) {
            std::string query =
                "INSERT IGNORE INTO seat_inventory "
                "(train_id, journey_date, class_type, seat_number, is_booked) "
                "VALUES (" +
                std::to_string(trainId) + ", '" +
                escapeSQL(date) + "', '" + dbClassType + "', " +
                std::to_string(seat) + ", FALSE)";

            if (mysql_query(connection, query.c_str()) != 0) {
                std::cerr << "Inventory initialization failed: "
                        << mysql_error(connection) << std::endl;
                return false;
            }
        }

        return true;
    }


    bool loadTrains() const {
        trainCache.clear();

        const char* query =
            "SELECT train_number, train_name, source, destination, "
            "base_fare, sleeper_capacity FROM trains ORDER BY train_number";

        if (mysql_query(connection, query) != 0)
            return false;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return false;

        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result)) != nullptr) {
            if (!row[0] || !row[1] || !row[2] ||
                !row[3] || !row[4] || !row[5]) {
                continue;
            }

            trainCache.emplace_back(
                std::stoi(row[0]),
                row[1],
                row[2],
                row[3],
                std::stod(row[4]),
                std::stoi(row[5])
            );
        }

        mysql_free_result(result);
        return true;
    }

public:
    explicit TrainRepository(
        MYSQL* dbConnection,
        std::mutex& sharedMutex
    )
        : connection(dbConnection),
        databaseMutex(sharedMutex) {

        if (connection == nullptr) {
            throw std::runtime_error(
                "TrainRepository received an invalid database connection."
            );
        }
    }

    // ADMIN: Add a train.
    bool addTrain(
        int trainNumber,
        const std::string& trainName,
        const std::string& source,
        const std::string& destination,
        double baseFare
    ) {
        if (trainNumber <= 0 || trainName.empty() ||
            source.empty() || destination.empty() ||
            baseFare <= 0 ||
            normalizeStation(source) == normalizeStation(destination)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(databaseMutex);

        std::string query =
            "INSERT INTO trains "
            "(train_number, train_name, source, destination, "
            "base_fare, ac_capacity, sleeper_capacity) VALUES (" +
            std::to_string(trainNumber) + ", '" +
            escapeSQL(trainName) + "', '" +
            escapeSQL(source) + "', '" +
            escapeSQL(destination) + "', " +
            std::to_string(baseFare) + ", 40, 100)";

        if (mysql_query(connection, query.c_str()) != 0)
            return false;

        trainCache.clear();
        return true;
    }

    // ADMIN: Delete a train.
    bool deleteTrain(int trainNumber) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        std::string query =
            "DELETE FROM trains WHERE train_number = " +
            std::to_string(trainNumber);

        if (mysql_query(connection, query.c_str()) != 0)
            return false;

        bool deleted = mysql_affected_rows(connection) == 1;

        if (deleted)
            trainCache.clear();

        return deleted;
    }

    // Search trains by source and destination.
    std::vector<Train> searchTrains(
        const std::string& source,
        const std::string& destination
    ) const {
        std::lock_guard<std::mutex> lock(databaseMutex);

        std::vector<Train> results;

        if (!loadTrains())
            return results;

        std::string from = normalizeStation(source);
        std::string to = normalizeStation(destination);

        for (const Train& train : trainCache) {
            if (normalizeStation(train.getSource()) == from &&
                normalizeStation(train.getDestination()) == to) {
                results.push_back(train);
            }
        }

        return results;
    }

    std::vector<Train> getAllTrains() const {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (!loadTrains())
            return {};

        return trainCache;
    }

    int getSeatCapacity(const std::string& classType) const {
        return capacity(classType);
    }

    // Get available seats for a train, date and class.
    int availableSeatsForTrain(
        int trainNumber,
        const std::string& journeyDate,
        const std::string& classType
    ) const {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (!validClass(classType) || journeyDate.empty())
            return 0;

        int trainId = getTrainId(trainNumber);
        if (trainId < 0)
            return 0;

        if (!initializeInventory(trainId, journeyDate, classType))
            return 0;

        std::string query =
            "SELECT COUNT(*) FROM seat_inventory "
            "WHERE train_id = " + std::to_string(trainId) +
            " AND journey_date = '" + escapeSQL(journeyDate) +
            "' AND class_type = '" + classType +
            "' AND is_booked = FALSE";

        if (mysql_query(connection, query.c_str()) != 0)
            return 0;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return 0;

        MYSQL_ROW row = mysql_fetch_row(result);
        int available = row && row[0] ? std::stoi(row[0]) : 0;

        mysql_free_result(result);
        return available;
    }

    bool isSeatAvailableForTrain(
        int trainNumber,
        const std::string& journeyDate,
        const std::string& classType,
        int seatNumber
    ) const {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (!validClass(classType) ||
            seatNumber < 1 || seatNumber > capacity(classType)) {
            return false;
        }

        int trainId = getTrainId(trainNumber);
        if (trainId < 0)
            return false;

        if (!initializeInventory(trainId, journeyDate, classType))
            return false;

        std::string query =
            "SELECT is_booked FROM seat_inventory "
            "WHERE train_id = " + std::to_string(trainId) +
            " AND journey_date = '" + escapeSQL(journeyDate) +
            "' AND class_type = '" + classType +
            "' AND seat_number = " + std::to_string(seatNumber);

        if (mysql_query(connection, query.c_str()) != 0)
            return false;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return false;

        MYSQL_ROW row = mysql_fetch_row(result);
        bool available = row && row[0] &&
                         std::string(row[0]) == "0";

        mysql_free_result(result);
        return available;
    }
    // Book a seat.
    bool bookSeatForTrain(
        int trainNumber,
        const std::string& journeyDate,
        const std::string& classType,
        int seatNumber,
        Train& bookedTrain
    ) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (!validClass(classType) ||
            seatNumber < 1 ||
            seatNumber > capacity(classType)) {
            return false;
        }

        int trainId = getTrainId(trainNumber);

        if (trainId < 0)
            return false;

        // Convert application class name to MySQL ENUM value.
        std::string dbClassType =
            (classType == "AC") ? "AC" : "SLEEPER";

        if (!initializeInventory(trainId, journeyDate, classType))
            return false;

        std::string query =
            "UPDATE seat_inventory SET is_booked = TRUE "
            "WHERE train_id = " + std::to_string(trainId) +
            " AND journey_date = '" + escapeSQL(journeyDate) +
            "' AND class_type = '" + dbClassType +
            "' AND seat_number = " + std::to_string(seatNumber) +
            " AND is_booked = FALSE";

        if (mysql_query(connection, query.c_str()) != 0) {
            std::cerr << "Seat booking failed: "
                    << mysql_error(connection) << std::endl;
            return false;
        }

        if (mysql_affected_rows(connection) != 1)
            return false;

        if (!loadTrains()) {
            // Roll back the seat reservation if train loading fails.
            std::string rollback =
                "UPDATE seat_inventory SET is_booked = FALSE "
                "WHERE train_id = " + std::to_string(trainId) +
                " AND journey_date = '" + escapeSQL(journeyDate) +
                "' AND class_type = '" + dbClassType +
                "' AND seat_number = " + std::to_string(seatNumber);

            mysql_query(connection, rollback.c_str());
            return false;
        }

        for (const Train& train : trainCache) {
            if (train.getTrainNumber() == trainNumber) {
                bookedTrain = train;
                return true;
            }
        }

        return false;
    }
    // Cancel/release a seat.
    bool cancelSeatForTrain(
        int trainNumber,
        const std::string& journeyDate,
        const std::string& classType,
        int seatNumber
    ) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (!validClass(classType) ||
            seatNumber < 1 ||
            seatNumber > capacity(classType)) {
            return false;
        }

        int trainId = getTrainId(trainNumber);

        if (trainId < 0)
            return false;

        // Convert application class name to database ENUM value.
        std::string dbClassType =
            (classType == "AC") ? "AC" : "SLEEPER";

        std::string query =
            "UPDATE seat_inventory SET is_booked = FALSE "
            "WHERE train_id = " + std::to_string(trainId) +
            " AND journey_date = '" + escapeSQL(journeyDate) +
            "' AND class_type = '" + dbClassType +
            "' AND seat_number = " + std::to_string(seatNumber) +
            " AND is_booked = TRUE";

        if (mysql_query(connection, query.c_str()) != 0) {
            std::cerr << "Seat release failed: "
                    << mysql_error(connection) << std::endl;
            return false;
        }

        return mysql_affected_rows(connection) == 1;
    }


    // Legacy methods retained for compatibility.
    bool bookSeatForTrain(int trainNumber, int seatNumber) {
        return false;
    }

    bool bookSeatForTrain(
        int trainNumber,
        int seatNumber,
        Train& bookedTrain
    ) {
        return false;
    }

    bool cancelSeatForTrain(int trainNumber, int seatNumber) {
        return false;
    }

    std::optional<Train> getTrainByNumber(int trainNumber) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (!loadTrains())
            return std::nullopt;

        for (const Train& train : trainCache) {
            if (train.getTrainNumber() == trainNumber)
                return train;
        }

        return std::nullopt;
    }
};

#endif
