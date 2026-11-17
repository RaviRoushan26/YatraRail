
#include "user.h"

#include <iostream>
#include <memory>

int main() {

    std::shared_ptr<User> user1 =
        std::make_shared<Passenger>("Ravi");

    std::shared_ptr<User> user2 =
        std::make_shared<PremiumPassenger>("Rahul");

    std::shared_ptr<User> user3 =
        std::make_shared<Admin>("Administrator");

    std::cout << "Username: "
              << user1->getUsername() << '\n';

    std::cout << "Role: "
              << user1->getRole() << '\n';

    std::cout << "\nUsername: "
              << user2->getUsername() << '\n';

    std::cout << "Role: "
              << user2->getRole() << '\n';

    // Demonstrate polymorphism through Passenger.
    std::shared_ptr<Passenger> premium =
        std::make_shared<PremiumPassenger>("Rahul");

    std::cout << "Discount: "
              << premium->getDiscount() * 100 << "%\n";

    std::cout << "\nAdmin Role: "
              << user3->getRole() << '\n';

    return 0;
}
