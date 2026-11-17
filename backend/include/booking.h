
#ifndef BOOKING_H
#define BOOKING_H

#include <string>

class Booking {
private:
    int pnr;
    std::string username;
    int trainNumber;
    std::string trainName;
    std::string source;
    std::string destination;
    std::string journeyDate;
    int seatNumber;
    std::string classType;
    double fare;
    bool cancelled;

public:
    Booking()
        : pnr(0),
          trainNumber(0),
          seatNumber(0),
          fare(0.0),
          cancelled(false) {}

    Booking(int pnr,
            const std::string& username,
            int trainNumber,
            const std::string& trainName,
            const std::string& source,
            const std::string& destination,
            const std::string& journeyDate,
            int seatNumber,
            const std::string& classType,
            double fare)
        : pnr(pnr),
          username(username),
          trainNumber(trainNumber),
          trainName(trainName),
          source(source),
          destination(destination),
          journeyDate(journeyDate),
          seatNumber(seatNumber),
          classType(classType),
          fare(fare),
          cancelled(false) {}

    int getPNR() const { return pnr; }
    std::string getUsername() const { return username; }
    int getTrainNumber() const { return trainNumber; }
    std::string getTrainName() const { return trainName; }
    std::string getSource() const { return source; }
    std::string getDestination() const { return destination; }
    std::string getJourneyDate() const { return journeyDate; }
    int getSeatNumber() const { return seatNumber; }
    std::string getClassType() const { return classType; }
    double getFare() const { return fare; }
    bool isCancelled() const { return cancelled; }

    void cancel() {
        cancelled = true;
    }

    void restore() {
        cancelled = false;
    }
};

#endif
