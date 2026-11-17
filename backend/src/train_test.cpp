
#include <iostream>
#include "train.h"

using namespace std;

int main()
{
    Train t1(
        16215,
        "Chamundi Express",
        "Mysore",
        "Bangalore",
        180,
        100
    );

    cout << "Train Name: " << t1.getTrainName() << endl;
    cout << "Train Number: " << t1.getTrainNumber() << endl;
    cout << "Source: " << t1.getSource() << endl;
    cout << "Destination: " << t1.getDestination() << endl;
    cout << "Available Seats: " << t1.availableSeats() << endl;

    t1.bookSeat(1);

    cout << "After booking seat 1: "
         << t1.availableSeats() << endl;

    t1.cancelSeat(1);

    cout << "After cancelling seat 1: "
         << t1.availableSeats() << endl;

    return 0;
}
