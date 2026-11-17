
#ifndef BOOKING_REPOSITORY_H
#define BOOKING_REPOSITORY_H

#include "booking.h"

#include <iostream>
#include <mysql.h>
#include <string>
#include <vector>
#include <mutex>
#include <random>
#include <stdexcept>



class BookingRepository {
private:
    MYSQL* connection;
    std::mutex& databaseMutex;

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

    int getUserId(const std::string& username) const {
        std::string query =
            "SELECT user_id FROM users WHERE username = '" +
            escapeSQL(username) + "' LIMIT 1";

        if (mysql_query(connection, query.c_str()) != 0)
            return -1;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return -1;

        MYSQL_ROW row = mysql_fetch_row(result);

        int userId = -1;
        if (row && row[0])
            userId = std::stoi(row[0]);

        mysql_free_result(result);
        return userId;
    }

    int getTrainId(int trainNumber) const {
        std::string query =
            "SELECT train_id FROM trains WHERE train_number = " +
            std::to_string(trainNumber) + " LIMIT 1";

        if (mysql_query(connection, query.c_str()) != 0)
            return -1;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return -1;

        MYSQL_ROW row = mysql_fetch_row(result);

        int trainId = -1;
        if (row && row[0])
            trainId = std::stoi(row[0]);

        mysql_free_result(result);
        return trainId;
    }

    int generatePNR() const {
        static thread_local std::mt19937 generator(
            std::random_device{}()
        );

        static thread_local std::uniform_int_distribution<int> distribution(
            100000000, 999999999
        );

        return distribution(generator);
    }

    bool readBookingQuery(
        const std::string& query,
        std::vector<Booking>& bookings
    ) const {
        if (mysql_query(connection, query.c_str()) != 0)
            return false;

        MYSQL_RES* result = mysql_store_result(connection);
        if (!result) return false;

        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result)) != nullptr) {
            if (!row[0] || !row[1] || !row[2] ||
                !row[3] || !row[4] || !row[5] ||
                !row[6] || !row[7] || !row[8] ||
                !row[9] || !row[10]) {
                continue;
            }

            Booking booking(
                std::stoi(row[0]),
                row[1],
                std::stoi(row[2]),
                row[3],
                row[4],
                row[5],
                row[6],
                std::stoi(row[7]),
                row[8],
                std::stod(row[9])
            );

            if (std::string(row[10]) == "CANCELLED")
                booking.cancel();

            bookings.push_back(booking);
        }

        mysql_free_result(result);
        return true;
    }

    std::string bookingSelectQuery() const {
        return
            "SELECT b.pnr, u.username, t.train_number, "
            "t.train_name, t.source, t.destination, "
            "b.journey_date, b.seat_number, b.class_type, "
            "b.fare, b.booking_status "
            "FROM bookings b "
            "JOIN users u ON b.user_id = u.user_id "
            "JOIN trains t ON b.train_id = t.train_id ";
    }

public:
    explicit BookingRepository(
        MYSQL* dbConnection,
        std::mutex& sharedMutex
    )
        : connection(dbConnection),
        databaseMutex(sharedMutex) {

        if (connection == nullptr) {
            throw std::runtime_error(
                "BookingRepository received an invalid database connection."
            );
        }
    }

    // Create a booking and return its PNR.
    // Returns -1 if booking creation fails.
    int createBooking(
        const std::string& username,
        int trainNumber,
        const std::string& trainName,
        const std::string& source,
        const std::string& destination,
        const std::string& journeyDate,
        int seatNumber,
        const std::string& classType,
        double fare
    ) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (username.empty() || trainNumber <= 0 ||
            seatNumber <= 0 || journeyDate.empty() ||
            fare < 0 ||
            (classType != "AC" && classType != "Sleeper")) {
            return -1;
        }

        int userId = getUserId(username);
        int trainId = getTrainId(trainNumber);

        if (userId < 0 || trainId < 0)
            return -1;

        for (int attempt = 0; attempt < 10; ++attempt) {
            int pnr = generatePNR();

            std::string query =
                "INSERT INTO bookings "
                "(pnr, user_id, train_id, journey_date, "
                "seat_number, class_type, fare, booking_status) "
                "VALUES (" +
                std::to_string(pnr) + ", " +
                std::to_string(userId) + ", " +
                std::to_string(trainId) + ", '" +
                escapeSQL(journeyDate) + "', " +
                std::to_string(seatNumber) + ", '" +
                classType + "', " +
                std::to_string(fare) + ", 'CONFIRMED')";

            if (mysql_query(connection, query.c_str()) == 0)
                return pnr;

            // Retry only if the generated PNR already exists.
            if (mysql_errno(connection) != 1062)
                return -1;
        }

        return -1;
    }

    // Retrieve all bookings belonging to a username.
    std::vector<Booking> getBookingsByUsername(
        const std::string& username
    ) const {
        std::lock_guard<std::mutex> lock(databaseMutex);

        std::vector<Booking> bookings;

        std::string query =
            bookingSelectQuery() +
            "WHERE u.username = '" + escapeSQL(username) +
            "' ORDER BY b.booking_time DESC";

        readBookingQuery(query, bookings);

        return bookings;
    }

    // Find a booking by PNR.
    bool getBookingByPNR(int pnr, Booking& booking) const {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (pnr <= 0)
            return false;

        std::vector<Booking> bookings;

        std::string query =
            bookingSelectQuery() +
            "WHERE b.pnr = " + std::to_string(pnr) +
            " LIMIT 1";

        if (!readBookingQuery(query, bookings) ||
            bookings.empty()) {
            return false;
        }

        booking = bookings.front();
        return true;
    }

    // Cancel a booking only if it belongs to the requesting user.
    bool cancelBooking(
        int pnr,
        const std::string& username,
        Booking& cancelledBooking
    ) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (pnr <= 0 || username.empty())
            return false;

        std::vector<Booking> bookings;

        std::string selectQuery =
            bookingSelectQuery() +
            "WHERE b.pnr = " + std::to_string(pnr) +
            " AND u.username = '" + escapeSQL(username) +
            "' AND b.booking_status = 'CONFIRMED' LIMIT 1";

        if (!readBookingQuery(selectQuery, bookings) ||
            bookings.empty()) {
            return false;
        }

        std::string updateQuery =
            "UPDATE bookings SET booking_status = 'CANCELLED' "
            "WHERE pnr = " + std::to_string(pnr) +
            " AND booking_status = 'CONFIRMED'";

        if (mysql_query(connection, updateQuery.c_str()) != 0)
            return false;

        if (mysql_affected_rows(connection) != 1)
            return false;

        cancelledBooking = bookings.front();
        cancelledBooking.cancel();

        return true;
    }

    // Retrieve every booking for admin use.
    std::vector<Booking> getAllBookings() const {
        std::lock_guard<std::mutex> lock(databaseMutex);

        std::vector<Booking> bookings;

        std::string query =
            bookingSelectQuery() +
            "ORDER BY b.booking_time DESC";

        readBookingQuery(query, bookings);

        return bookings;
    }

    bool restoreBooking(int pnr, const std::string& username) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        int userId = getUserId(username);

        if (userId <= 0)
            return false;

        std::string query =
            "UPDATE bookings "
            "SET booking_status = 'CONFIRMED' "
            "WHERE pnr = " + std::to_string(pnr) +
            " AND user_id = " + std::to_string(userId) +
            " AND booking_status = 'CANCELLED'";

        if (mysql_query(connection, query.c_str()) != 0) {
            std::cerr << "Booking restoration failed: "
                    << mysql_error(connection) << std::endl;
            return false;
        }

        return mysql_affected_rows(connection) == 1;
    }


    bool hasBookingsForTrain(int trainNumber) {
        std::lock_guard<std::mutex> lock(databaseMutex);

        if (trainNumber <= 0)
            return false;

        std::string query =
            "SELECT COUNT(*) FROM bookings b "
            "JOIN trains t ON b.train_id = t.train_id "
            "WHERE t.train_number = " +
            std::to_string(trainNumber);

        if (mysql_query(connection, query.c_str()) != 0)
            return true; // Safely prevent deletion if query fails

        MYSQL_RES* result = mysql_store_result(connection);

        if (!result)
            return true;

        MYSQL_ROW row = mysql_fetch_row(result);

        bool hasBookings = row && row[0] &&
                           std::stoll(row[0]) > 0;

        mysql_free_result(result);

        return hasBookings;
    }
};

#endif
