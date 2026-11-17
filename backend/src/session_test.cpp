
#include <iostream>
#include "session_repository.h"

int main() {
    SessionRepository repository;

    std::string token = repository.createSession(
        "RaviRoushan",
        "Premium Passenger"
    );

    if (token.empty()) {
        std::cout << "Session creation failed.\n";
        return 1;
    }

    std::cout << "Session created successfully!\n";
    std::cout << "Token: " << token << "\n";

    SessionInfo info;

    if (repository.getSession(token, info)) {
        std::cout << "Username: " << info.username << "\n";
        std::cout << "Role: " << info.role << "\n";
    }

    repository.deleteSession(token);

    if (!repository.getSession(token, info)) {
        std::cout << "Session deleted successfully!\n";
    }

    return 0;
}
