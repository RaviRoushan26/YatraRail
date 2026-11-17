
#ifndef USER_H
#define USER_H

#include <string>

class User {
protected:
    std::string username;

public:
    explicit User(const std::string& name)
        : username(name) {}

    virtual std::string getRole() const = 0;

    virtual ~User() = default;

    std::string getUsername() const {
        return username;
    }
};

class Passenger : public User {
public:
    explicit Passenger(const std::string& name)
        : User(name) {}

    std::string getRole() const override {
        return "Passenger";
    }

    virtual double getDiscount() const {
        return 0.0;
    }

    virtual ~Passenger() = default;
};

// Multilevel inheritance:
// User -> Passenger -> PremiumPassenger

class PremiumPassenger : public Passenger {
public:
    explicit PremiumPassenger(const std::string& name)
        : Passenger(name) {}

    std::string getRole() const override {
        return "Premium Passenger";
    }

    double getDiscount() const override {
        return 0.10;
    }
};

class Admin : public User {
public:
    explicit Admin(const std::string& name)
        : User(name) {}

    std::string getRole() const override {
        return "Admin";
    }
};

#endif
