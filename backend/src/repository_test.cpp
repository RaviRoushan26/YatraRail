
#include <iostream>
#include <string>
#include <vector>
#include "database.h"
#include "train_repository.h"

using namespace std;

int main() {

    Database database;

    if (!database.isConnected()) {
        cout << "Database connection failed!\n";
        return 1;
    }

    TrainRepository repository(
        database.getConnection(),
        database.getMutex()
    );

    string source = "Mysuru";
    string destination = "Bengaluru";

    vector<Train> results =
        repository.searchTrains(source, destination);

    cout << "\nMatching trains:\n";

    if (results.empty()) {
        cout << "No trains found.\n";
    }

    for (const Train& train : results) {

        cout << "\nTrain Number: "
             << train.getTrainNumber();

        cout << "\nTrain Name: "
             << train.getTrainName();

        cout << "\nSource: "
             << train.getSource();

        cout << "\nDestination: "
             << train.getDestination();

        cout << "\nFare: Rs. "
             << train.getBaseFare();

        cout << "\n-------------------------\n";
    }

    return 0;
}
