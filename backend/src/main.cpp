
#include "crow.h"
#include "train_repository.h"
#include "user.h"
#include "user_repository.h"
#include "session_repository.h"
#include "booking_repository.h"
#include "database.h"
#include <stdexcept>

#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <utility>
#include <set>


// ---------------- AUTHENTICATION HELPERS ----------------

bool getAuthenticatedSession(
    const crow::request& req,
    SessionRepository& sessionRepository,
    SessionInfo& session
) {
    std::string header = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";

    if (header.compare(0, prefix.size(), prefix) != 0)
        return false;

    return sessionRepository.getSession(
        header.substr(prefix.size()),
        session
    );
}

bool isAdmin(
    const crow::request& req,
    SessionRepository& sessionRepository,
    SessionInfo& session
) {
    return getAuthenticatedSession(req, sessionRepository, session)
        && session.role == "Admin";
}

// ---------------- DATE VALIDATION ----------------

bool isValidDate(const std::string& date) {

    std::tm requested = {};
    std::istringstream input(date);

    input >> std::get_time(&requested, "%Y-%m-%d");

    if (input.fail() ||
        input.peek() != std::char_traits<char>::eof()) {
        return false;
    }

    int year = requested.tm_year;
    int month = requested.tm_mon;
    int day = requested.tm_mday;

    std::tm normalized = requested;
    normalized.tm_isdst = -1;

    if (std::mktime(&normalized) == -1)
        return false;

    return normalized.tm_year == year
        && normalized.tm_mon == month
        && normalized.tm_mday == day;
}

// ---------------- CORS MIDDLEWARE ----------------

struct CorsMiddleware {

    struct context {};

    void before_handle(
        crow::request& req,
        crow::response& res,
        context&
    ) {
        res.set_header(
            "Access-Control-Allow-Origin",
            "http://localhost:5500"
        );

        res.set_header(
            "Access-Control-Allow-Methods",
            "GET, POST, DELETE, OPTIONS"
        );

        res.set_header(
            "Access-Control-Allow-Headers",
            "Content-Type, Authorization"
        );

        if (req.method == crow::HTTPMethod::Options) {
            res.code = 204;
            res.end();
        }
    }

    void after_handle(
        crow::request&,
        crow::response& res,
        context&
    ) {
        res.set_header(
            "Access-Control-Allow-Origin",
            "http://localhost:5500"
        );

        res.set_header(
            "Access-Control-Allow-Methods",
            "GET, POST, DELETE, OPTIONS"
        );

        res.set_header(
            "Access-Control-Allow-Headers",
            "Content-Type, Authorization"
        );
    }
};

// ---------------- MAIN ----------------

int main() {
    Database database;

    if (!database.isConnected()) {
        std::cerr << "Server stopped: database unavailable.\n";
        return 1;
    }

    UserRepository userRepository(
        database.getConnection(),
        database.getMutex()
    );

    crow::App<CorsMiddleware> app;

    TrainRepository repository(
        database.getConnection(),
        database.getMutex()
    );
    BookingRepository bookingRepository(
        database.getConnection(),
        database.getMutex()
    );

    SessionRepository sessionRepository;

    // ---------------- HOME ----------------

    // ---------------- HOME ----------------

    CROW_ROUTE(app, "/")
    ([]() {
        return "Railway Reservation System Server is Running!";
    });

    // =====================================================
    //                    PASSENGER APIs
    // =====================================================

    // ---------------- REGISTER ----------------

    CROW_ROUTE(app, "/api/auth/register")
    .methods(crow::HTTPMethod::Post)
    ([&userRepository](const crow::request& req) {

        crow::json::wvalue result;

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("username") ||
            !body.has("password") ||
            body["username"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

            result["success"] = false;
            result["message"] = "Username and password are required.";

            return crow::response(400, result);
        }

        std::string username = body["username"].s();
        std::string password = body["password"].s();

        if (username.empty() || password.size() < 8) {

            result["success"] = false;
            result["message"] =
                "Username is required and password must contain at least 8 characters.";

            return crow::response(400, result);
        }

        bool premium = false;

        if (body.has("premium") &&
            body["premium"].t() == crow::json::type::True) {
            premium = true;
        }

        if (!userRepository.registerUser(
                username, password, premium)) {

            result["success"] = false;
            result["message"] =
                "Registration failed. Username may already exist.";

            return crow::response(409, result);
        }

        result["success"] = true;
        result["message"] = "Registration successful.";
        result["username"] = username;
        result["role"] = premium
            ? "Premium Passenger"
            : "Passenger";

        return crow::response(201, result);
    });

    // ---------------- PASSENGER LOGIN ----------------

    CROW_ROUTE(app, "/api/auth/login")
    .methods(crow::HTTPMethod::Post)
    ([&userRepository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("username") ||
            !body.has("password") ||
            body["username"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

            result["success"] = false;
            result["message"] = "Username and password are required.";

            return crow::response(400, result);
        }

        std::string username = body["username"].s();
        std::string password = body["password"].s();

        if (!userRepository.authenticate(username, password)) {

            result["success"] = false;
            result["message"] = "Invalid username or password.";

            return crow::response(401, result);
        }

        std::string role = userRepository.getRole(username);

        std::string token =
            sessionRepository.createSession(username, role);

        if (token.empty()) {

            result["success"] = false;
            result["message"] = "Could not create login session.";

            return crow::response(500, result);
        }

        result["success"] = true;
        result["message"] = "Login successful.";
        result["username"] = username;
        result["role"] = role;
        result["token"] = token;

        return crow::response(200, result);
    });

    // ---------------- CHANGE PASSWORD ----------------

    CROW_ROUTE(app, "/api/auth/change-password")
    .methods(crow::HTTPMethod::Post)
    ([&userRepository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!getAuthenticatedSession(
                req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Please log in first.";

            return crow::response(401, result);
        }

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("currentPassword") ||
            !body.has("newPassword") ||
            body["currentPassword"].t() != crow::json::type::String ||
            body["newPassword"].t() != crow::json::type::String) {

            result["success"] = false;
            result["message"] =
                "Current and new passwords are required.";

            return crow::response(400, result);
        }

        std::string currentPassword =
            body["currentPassword"].s();

        std::string newPassword =
            body["newPassword"].s();

        if (newPassword.size() < 8) {

            result["success"] = false;
            result["message"] =
                "New password must contain at least 8 characters.";

            return crow::response(400, result);
        }

        if (!userRepository.changePassword(
                session.username,
                currentPassword,
                newPassword)) {

            result["success"] = false;
            result["message"] =
                "Password change failed. Check your current password.";

            return crow::response(400, result);
        }

        result["success"] = true;
        result["message"] = "Password changed successfully.";

        return crow::response(200, result);
    });

    // ---------------- LOGOUT ----------------

    CROW_ROUTE(app, "/api/auth/logout")
    .methods(crow::HTTPMethod::Post)
    ([&sessionRepository](const crow::request& req) {

        crow::json::wvalue result;

        std::string header =
            req.get_header_value("Authorization");

        const std::string prefix = "Bearer ";

        if (header.compare(0, prefix.size(), prefix) != 0) {

            result["success"] = false;
            result["message"] = "Session token is missing.";

            return crow::response(401, result);
        }

        std::string token = header.substr(prefix.size());

        if (!sessionRepository.deleteSession(token)) {

            result["success"] = false;
            result["message"] = "Invalid or expired session.";

            return crow::response(401, result);
        }

        result["success"] = true;
        result["message"] = "Logout successful.";

        return crow::response(200, result);
    });

    // ---------------- TRAIN SEARCH ----------------

    CROW_ROUTE(app, "/api/trains/search")
    ([&repository](const crow::request& req) {

        crow::json::wvalue result;

        const char* source = req.url_params.get("source");
        const char* destination = req.url_params.get("destination");
        const char* date = req.url_params.get("date");

        if (!source || !destination || !date ||
            std::string(source).empty() ||
            std::string(destination).empty() ||
            std::string(date).empty()) {

            result["success"] = false;
            result["message"] = "All search fields are required.";

            return crow::response(400, result);
        }

        std::string from = source;
        std::string to = destination;
        std::string journeyDate = date;

        if (from == to || !isValidDate(journeyDate)) {

            result["success"] = false;
            result["message"] =
                "Invalid source, destination or journey date.";

            return crow::response(400, result);
        }

        auto trains = repository.searchTrains(from, to);

        crow::json::wvalue::list trainList;

        for (const Train& train : trains) {

            crow::json::wvalue item;

            item["trainNumber"] = train.getTrainNumber();
            item["trainName"] = train.getTrainName();
            item["source"] = train.getSource();
            item["destination"] = train.getDestination();
            item["fare"] = train.getBaseFare();

            item["availableSeatsAC"] =
                repository.availableSeatsForTrain(
                    train.getTrainNumber(),
                    journeyDate,
                    "AC"
                );

            item["totalSeatsAC"] =
                repository.getSeatCapacity("AC");

            item["availableSeatsSleeper"] =
                repository.availableSeatsForTrain(
                    train.getTrainNumber(),
                    journeyDate,
                    "Sleeper"
                );

            item["totalSeatsSleeper"] =
                repository.getSeatCapacity("Sleeper");

            trainList.emplace_back(std::move(item));
        }

        result["success"] = true;
        result["source"] = from;
        result["destination"] = to;
        result["journeyDate"] = journeyDate;
        result["trains"] = std::move(trainList);
        result["message"] = trains.empty()
            ? "No matching trains found."
            : "Matching trains retrieved successfully.";

        return crow::response(200, result);
    });

    // ---------------- PUBLIC STATION LIST ----------------

    CROW_ROUTE(app, "/api/trains/stations")
    ([&repository]() {

        crow::json::wvalue result;
        crow::json::wvalue::list stationList;

        std::set<std::string> uniqueStations;

        auto trains = repository.getAllTrains();

        for (const Train& train : trains) {
            uniqueStations.insert(train.getSource());
            uniqueStations.insert(train.getDestination());
        }

        for (const auto& station : uniqueStations) {
            stationList.emplace_back(station);
        }

        result["success"] = true;
        result["stations"] = std::move(stationList);

        return crow::response(200, result);
    });

    // ---------------- BOOK TICKET ----------------

    CROW_ROUTE(app, "/api/bookings")
    .methods(crow::HTTPMethod::Post)
    ([&repository, &bookingRepository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!getAuthenticatedSession(
                req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Please log in to book a ticket.";

            return crow::response(401, result);
        }

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("trainNumber") ||
            !body.has("seatNumber") ||
            !body.has("journeyDate") ||
            !body.has("classType")) {

            result["success"] = false;
            result["message"] = "All booking fields are required.";

            return crow::response(400, result);
        }

        if (body["trainNumber"].t() != crow::json::type::Number ||
            body["seatNumber"].t() != crow::json::type::Number ||
            body["journeyDate"].t() != crow::json::type::String ||
            body["classType"].t() != crow::json::type::String) {

            result["success"] = false;
            result["message"] = "Invalid booking field types.";

            return crow::response(400, result);
        }

        int trainNumber = body["trainNumber"].i();
        int seatNumber = body["seatNumber"].i();

        std::string journeyDate = body["journeyDate"].s();
        std::string classType = body["classType"].s();

        if (trainNumber <= 0 ||
            !isValidDate(journeyDate) ||
            (classType != "AC" && classType != "Sleeper")) {

            result["success"] = false;
            result["message"] = "Invalid booking details.";

            return crow::response(400, result);
        }

        int maxSeats = classType == "AC" ? 40 : 100;

        if (seatNumber < 1 || seatNumber > maxSeats) {

            result["success"] = false;
            result["message"] = "Invalid seat number.";

            return crow::response(400, result);
        }

        Train bookedTrain;

        if (!repository.bookSeatForTrain(
                trainNumber,
                journeyDate,
                classType,
                seatNumber,
                bookedTrain)) {

            result["success"] = false;
            result["message"] =
                "Train not found or selected seat is unavailable.";

            return crow::response(409, result);
        }

        double fare = bookedTrain.getBaseFare();

        if (classType == "AC")
            fare *= 1.8;

        if (session.role == "Premium Passenger")
            fare *= 0.90;

        int pnr = bookingRepository.createBooking(
            session.username,
            trainNumber,
            bookedTrain.getTrainName(),
            bookedTrain.getSource(),
            bookedTrain.getDestination(),
            journeyDate,
            seatNumber,
            classType,
            fare
        );

        if (pnr == -1) {

            // Release the seat because booking creation failed
            repository.cancelSeatForTrain(
                trainNumber,
                journeyDate,
                classType,
                seatNumber
            );

            result["success"] = false;
            result["message"] = "Booking failed. Please try again.";

            return crow::response(500, result);
        }

        result["success"] = true;
        result["message"] = "Ticket booked successfully!";
        result["pnr"] = pnr;
        result["username"] = session.username;
        result["trainNumber"] = trainNumber;
        result["trainName"] = bookedTrain.getTrainName();
        result["source"] = bookedTrain.getSource();
        result["destination"] = bookedTrain.getDestination();
        result["journeyDate"] = journeyDate;
        result["seatNumber"] = seatNumber;
        result["classType"] = classType;
        result["fare"] = fare;

        return crow::response(201, result);
    });

    // ---------------- MY BOOKINGS ----------------

    CROW_ROUTE(app, "/api/bookings")
    .methods(crow::HTTPMethod::Get)
    ([&bookingRepository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!getAuthenticatedSession(
                req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Please log in first.";

            return crow::response(401, result);
        }

        auto bookings =
            bookingRepository.getBookingsByUsername(session.username);

        crow::json::wvalue::list list;

        for (const Booking& booking : bookings) {

            crow::json::wvalue item;

            item["pnr"] = booking.getPNR();
            item["trainNumber"] = booking.getTrainNumber();
            item["trainName"] = booking.getTrainName();
            item["source"] = booking.getSource();
            item["destination"] = booking.getDestination();
            item["journeyDate"] = booking.getJourneyDate();
            item["seatNumber"] = booking.getSeatNumber();
            item["classType"] = booking.getClassType();
            item["fare"] = booking.getFare();

            item["status"] = booking.isCancelled()
                ? "Cancelled"
                : "Confirmed";

            list.emplace_back(std::move(item));
        }

        result["success"] = true;
        result["bookings"] = std::move(list);
        result["message"] = "Bookings retrieved successfully.";

        return crow::response(200, result);
    });

    // ---------------- PNR LOOKUP ----------------

    CROW_ROUTE(app, "/api/bookings/pnr")
    .methods(crow::HTTPMethod::Get)
    ([&bookingRepository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!getAuthenticatedSession(
                req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Please log in first.";

            return crow::response(401, result);
        }

        std::string value = req.url_params.get("pnr");

        int pnr = 0;

        try {
            size_t position = 0;
            pnr = std::stoi(value, &position);

            if (value.empty() ||
                position != value.size() ||
                pnr <= 0) {
                throw std::invalid_argument("Invalid PNR");
            }
        }
        catch (...) {

            result["success"] = false;
            result["message"] = "Invalid PNR format.";

            return crow::response(400, result);
        }

        Booking booking;

        if (!bookingRepository.getBookingByPNR(pnr, booking)) {

            result["success"] = false;
            result["message"] = "No booking found for this PNR.";

            return crow::response(404, result);
        }

        if (booking.getUsername() != session.username) {

            result["success"] = false;
            result["message"] =
                "You are not authorized to view this booking.";

            return crow::response(403, result);
        }

        result["success"] = true;
        result["message"] = "PNR details retrieved successfully.";
        result["pnr"] = booking.getPNR();
        result["username"] = booking.getUsername();
        result["trainNumber"] = booking.getTrainNumber();
        result["trainName"] = booking.getTrainName();
        result["source"] = booking.getSource();
        result["destination"] = booking.getDestination();
        result["journeyDate"] = booking.getJourneyDate();
        result["seatNumber"] = booking.getSeatNumber();
        result["classType"] = booking.getClassType();
        result["fare"] = booking.getFare();
        result["status"] = booking.isCancelled()
            ? "Cancelled"
            : "Confirmed";

        return crow::response(200, result);
    });

    // ---------------- CANCEL BOOKING ----------------

    CROW_ROUTE(app, "/api/bookings/cancel")
    .methods(crow::HTTPMethod::Post)
    ([&bookingRepository, &sessionRepository, &repository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!getAuthenticatedSession(
                req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Please log in first.";

            return crow::response(401, result);
        }

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("pnr") ||
            body["pnr"].t() != crow::json::type::Number) {

            result["success"] = false;
            result["message"] = "Please provide a valid PNR.";

            return crow::response(400, result);
        }

        int pnr = body["pnr"].i();

        if (pnr <= 0) {

            result["success"] = false;
            result["message"] = "Invalid PNR number.";

            return crow::response(400, result);
        }

        Booking cancelledBooking;

        if (!bookingRepository.cancelBooking(
                pnr,
                session.username,
                cancelledBooking)) {

            result["success"] = false;
            result["message"] =
                "Booking not found, already cancelled, or not owned by you.";

            return crow::response(404, result);
        }

        bool seatReleased = repository.cancelSeatForTrain(
            cancelledBooking.getTrainNumber(),
            cancelledBooking.getJourneyDate(),
            cancelledBooking.getClassType(),
            cancelledBooking.getSeatNumber()
        );

        if (!seatReleased) {

            bool restored = bookingRepository.restoreBooking(
                pnr,
                session.username
            );

            result["success"] = false;
            result["message"] = restored
                ? "Cancellation failed. Your booking has been restored."
                : "Cancellation and booking restoration failed. Contact admin.";

            return crow::response(500, result);
        }

        result["success"] = true;
        result["message"] = "Ticket cancelled successfully!";
        result["pnr"] = pnr;
        result["trainName"] = cancelledBooking.getTrainName();
        result["trainNumber"] = cancelledBooking.getTrainNumber();
        result["seatNumber"] = cancelledBooking.getSeatNumber();
        result["journeyDate"] = cancelledBooking.getJourneyDate();
        result["status"] = "Cancelled";

        return crow::response(200, result);
    });

    // =====================================================
    //                       ADMIN APIs
    // =====================================================

    // ---------------- ADMIN LOGIN ----------------

    CROW_ROUTE(app, "/api/admin/login")
    .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {

        crow::json::wvalue result;

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("username") ||
            !body.has("password") ||
            body["username"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

            result["success"] = false;
            result["message"] = "Admin username and password are required.";

            return crow::response(400, result);
        }

        const char* configuredUsername =
            std::getenv("RAILBOOK_ADMIN_USERNAME");

        const char* configuredPassword =
            std::getenv("RAILBOOK_ADMIN_PASSWORD");

        if (!configuredUsername ||
            !configuredPassword ||
            std::string(configuredPassword).size() < 12) {

            result["success"] = false;
            result["message"] =
                "Admin credentials are not configured securely on the server.";

            return crow::response(503, result);
        }

        std::string username = body["username"].s();
        std::string password = body["password"].s();

        if (username != configuredUsername ||
            password != configuredPassword) {

            result["success"] = false;
            result["message"] = "Invalid admin credentials.";

            return crow::response(401, result);
        }

        // Admin identity is not registered as a passenger.
        // This route is bound to the session repository below.
        result["success"] = true;
        result["username"] = username;
        result["role"] = "Admin";

        return crow::response(200, result);
    });

    // ---------------- ADMIN LOGIN SESSION ----------------
    // Separate route creates an authenticated admin session.

    CROW_ROUTE(app, "/api/admin/session")
    .methods(crow::HTTPMethod::Post)
    ([&sessionRepository](const crow::request& req) {

        crow::json::wvalue result;

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("username") ||
            !body.has("password") ||
            body["username"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

            result["success"] = false;
            result["message"] = "Admin credentials are required.";

            return crow::response(400, result);
        }

        const char* configuredUsername =
            std::getenv("RAILBOOK_ADMIN_USERNAME");

        const char* configuredPassword =
            std::getenv("RAILBOOK_ADMIN_PASSWORD");

        if (!configuredUsername ||
            !configuredPassword ||
            std::string(configuredPassword).size() < 12 ||
            body["username"].s() != configuredUsername ||
            body["password"].s() != configuredPassword) {

            result["success"] = false;
            result["message"] = "Invalid admin credentials.";

            return crow::response(401, result);
        }

        std::string token = sessionRepository.createSession(
            configuredUsername,
            "Admin"
        );

        if (token.empty()) {

            result["success"] = false;
            result["message"] = "Could not create admin session.";

            return crow::response(500, result);
        }

        result["success"] = true;
        result["message"] = "Admin login successful.";
        result["username"] = configuredUsername;
        result["role"] = "Admin";
        result["token"] = token;

        return crow::response(200, result);
    });

    // ---------------- ADMIN: LIST TRAINS ----------------

    CROW_ROUTE(app, "/api/admin/trains")
    .methods(crow::HTTPMethod::Get)
    ([&repository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!isAdmin(req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Admin access required.";

            return crow::response(403, result);
        }

        crow::json::wvalue::list list;

        auto trains = repository.getAllTrains();

        for (const Train& train : trains) {

            crow::json::wvalue item;

            item["trainNumber"] = train.getTrainNumber();
            item["trainName"] = train.getTrainName();
            item["source"] = train.getSource();
            item["destination"] = train.getDestination();
            item["fare"] = train.getBaseFare();

            list.emplace_back(std::move(item));
        }

        result["success"] = true;
        result["trains"] = std::move(list);

        return crow::response(200, result);
    });

    // ---------------- ADMIN: ADD TRAIN ----------------

    CROW_ROUTE(app, "/api/admin/trains")
    .methods(crow::HTTPMethod::Post)
    ([&repository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!isAdmin(req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Admin access required.";

            return crow::response(403, result);
        }

        auto body = crow::json::load(req.body);

        if (!body ||
            !body.has("trainNumber") ||
            !body.has("trainName") ||
            !body.has("source") ||
            !body.has("destination") ||
            !body.has("fare")) {

            result["success"] = false;
            result["message"] = "All train fields are required.";

            return crow::response(400, result);
        }

        if (body["trainNumber"].t() != crow::json::type::Number ||
            body["trainName"].t() != crow::json::type::String ||
            body["source"].t() != crow::json::type::String ||
            body["destination"].t() != crow::json::type::String ||
            body["fare"].t() != crow::json::type::Number) {

            result["success"] = false;
            result["message"] = "Invalid train field types.";

            return crow::response(400, result);
        }

        int number = body["trainNumber"].i();
        std::string name = body["trainName"].s();
        std::string source = body["source"].s();
        std::string destination = body["destination"].s();
        double fare = body["fare"].d();

        if (number <= 0 ||
            name.empty() ||
            source.empty() ||
            destination.empty() ||
            fare <= 0) {

            result["success"] = false;
            result["message"] = "Invalid train details.";

            return crow::response(400, result);
        }

        if (!repository.addTrain(
                number,
                name,
                source,
                destination,
                fare)) {

            result["success"] = false;
            result["message"] =
                "Train could not be added. Train number may already exist.";

            return crow::response(409, result);
        }

        result["success"] = true;
        result["message"] = "Train added successfully.";
        result["trainNumber"] = number;

        return crow::response(201, result);
    });

    // ---------------- ADMIN: DELETE TRAIN ----------------

    CROW_ROUTE(app, "/api/admin/trains/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&repository, &bookingRepository, &sessionRepository]
    (const crow::request& req, int trainNumber) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!isAdmin(req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Admin access required.";

            return crow::response(403, result);
        }

        if (trainNumber <= 0) {

            result["success"] = false;
            result["message"] = "Invalid train number.";

            return crow::response(400, result);
        }

        if (bookingRepository.hasBookingsForTrain(trainNumber)) {

            result["success"] = false;
            result["message"] =
                "This train has booking history and cannot be deleted.";

            return crow::response(409, result);
        }

        if (!repository.deleteTrain(trainNumber)) {

            result["success"] = false;
            result["message"] = "Train not found.";

            return crow::response(404, result);
        }

        result["success"] = true;
        result["message"] = "Train deleted successfully.";

        return crow::response(200, result);
    });

    // ---------------- ADMIN: VIEW ALL BOOKINGS ----------------

    CROW_ROUTE(app, "/api/admin/bookings")
    .methods(crow::HTTPMethod::Get)
    ([&bookingRepository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!isAdmin(req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Admin access required.";

            return crow::response(403, result);
        }

        auto bookings = bookingRepository.getAllBookings();

        crow::json::wvalue::list list;

        for (const Booking& booking : bookings) {

            crow::json::wvalue item;

            item["pnr"] = booking.getPNR();
            item["username"] = booking.getUsername();
            item["trainNumber"] = booking.getTrainNumber();
            item["trainName"] = booking.getTrainName();
            item["source"] = booking.getSource();
            item["destination"] = booking.getDestination();
            item["journeyDate"] = booking.getJourneyDate();
            item["seatNumber"] = booking.getSeatNumber();
            item["classType"] = booking.getClassType();
            item["fare"] = booking.getFare();

            item["status"] = booking.isCancelled()
                ? "Cancelled"
                : "Confirmed";

            list.emplace_back(std::move(item));
        }

        result["success"] = true;
        result["bookings"] = std::move(list);

        return crow::response(200, result);
    });

    // ---------------- ADMIN: VIEW PASSENGERS ----------------

    CROW_ROUTE(app, "/api/admin/passengers")
    .methods(crow::HTTPMethod::Get)
    ([&userRepository, &sessionRepository]
    (const crow::request& req) {

        crow::json::wvalue result;

        SessionInfo session;

        if (!isAdmin(req, sessionRepository, session)) {

            result["success"] = false;
            result["message"] = "Admin access required.";

            return crow::response(403, result);
        }

        auto usernames = userRepository.getAllUsernames();

        crow::json::wvalue::list list;

        for (const std::string& username : usernames) {

            crow::json::wvalue item;

            item["username"] = username;
            item["role"] = userRepository.getRole(username);

            list.emplace_back(std::move(item));
        }

        result["success"] = true;
        result["passengers"] = std::move(list);

        return crow::response(200, result);
    });

    // ---------------- START SERVER ----------------

    app.port(18080).run();

    return 0;
}
