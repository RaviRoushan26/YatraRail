
#ifndef SEAT_H
#define SEAT_H

class Seat {
private:
    int seatNumber;
    bool booked;

public:
    explicit Seat(int number = 0)
        : seatNumber(number), booked(false) {}

    int getSeatNumber() const {
        return seatNumber;
    }

    bool isBooked() const {
        return booked;
    }

    bool book() {
        if (booked) return false;

        booked = true;
        return true;
    }

    void cancel() {
        booked = false;
    }
};

#endif
