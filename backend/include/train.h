
#ifndef TRAIN_H
#define TRAIN_H

#include "seat.h"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

class Train {
private:
    int trainNumber;
    std::string trainName;
    std::string source;
    std::string destination;
    double baseFare;

    // Composition: Train owns its Seat objects.
    std::vector<Seat> seats;

public:
    Train()
        : trainNumber(0), baseFare(0) {}

    Train(int number, const std::string& name,
          const std::string& from, const std::string& to,
          double fare, int seatCount)
        : trainNumber(number),
          trainName(name),
          source(from),
          destination(to),
          baseFare(fare) {

        for (int i = 1; i <= seatCount; ++i) {
            seats.emplace_back(i);
        }
    }

    int getTrainNumber() const {
        return trainNumber;
    }

    std::string getTrainName() const {
        return trainName;
    }

    std::string getSource() const {
        return source;
    }

    std::string getDestination() const {
        return destination;
    }

    double getBaseFare() const {
        return baseFare;
    }

    int getSeatCount() const {
        return static_cast<int>(seats.size());
    }

    int availableSeats() const {
        int count = 0;

        for (const Seat& seat : seats) {
            if (!seat.isBooked()) {
                ++count;
            }
        }

        return count;
    }

    bool isSeatAvailable(int seatNumber) const {
        if (seatNumber < 1 || seatNumber > getSeatCount())
            return false;

        return !seats[seatNumber - 1].isBooked();
    }

    bool bookSeat(int seatNumber) {
        if (seatNumber < 1 || seatNumber > getSeatCount())
            return false;

        return seats[seatNumber - 1].book();
    }

    bool cancelSeat(int seatNumber) {
        if (seatNumber < 1 || seatNumber > getSeatCount())
            return false;

        if (!seats[seatNumber - 1].isBooked())
            return false;

        seats[seatNumber - 1].cancel();
        return true;
    }

    void display() const {
        std::cout << std::left
                  << std::setw(10) << trainNumber
                  << std::setw(24) << trainName
                  << std::setw(18) << source
                  << std::setw(18) << destination
                  << std::setw(12) << std::fixed
                  << std::setprecision(2) << baseFare
                  << availableSeats() << "/" << getSeatCount()
                  << '\n';
    }

    void restoreBookedSeat(int seatNumber) {
        if (seatNumber >= 1 && seatNumber <= getSeatCount()) {
            seats[seatNumber - 1].book();
        }
    }
};

#endif
