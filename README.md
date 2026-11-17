# YatraRail – Railway Reservation Web Application

A web-based railway reservation system developed using **C++, Crow, MySQL, HTML, CSS and JavaScript**.

YatraRail provides passenger railway reservation functionality along with an administrative interface for managing railway operations.

---

## 1. Project Overview

YatraRail is a Railway Reservation Web Application developed as an academic project at **JSS Science and Technology University (JSSSTU), Mysuru**, under the Information Science and Engineering department.

The application demonstrates the integration of a C++ backend, MySQL database and web-based frontend to perform railway reservation operations.

The project is designed to provide a simple interface for passengers to search for trains, book tickets, check PNR status and cancel reservations.

---

## 2. Features

### Passenger Module

- Passenger registration
- Passenger login and logout
- Train search
- Ticket booking
- PNR status lookup
- Ticket cancellation

### Admin Module

- Admin login and logout
- Administrative dashboard
- View all trains
- Add new trains
- Delete trains
- View all passenger bookings
- View registered passengers

---

## 3. Technology Stack

| Component | Technology |
|---|---|
| Programming Language | C++ |
| Backend Framework | Crow |
| Database | MySQL |
| Database Connector | MariaDB Connector/C |
| Frontend | HTML, CSS, JavaScript |
| Build System | CMake |
| Build Tool | Ninja |
| Compiler | GCC / MinGW-w64 |
| IDE | Visual Studio Code |
| Operating System | Windows |

---

## 4. Project Directory Structure

```text
RailwayReservationWeb/
│
├── .vscode/
│
├── backend/
│   ├── include/
│   ├── src/
│   ├── third_party/
│   │   └── Crow/
│   ├── CMakeLists.txt
│   └── build/              (Generated during compilation)
│
├── database/
│   └── railway_schema.sql
│
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── script.js
│
├── .gitignore
└── README.md
```

*Note: This is a simplified overview of the project structure.*

---

## 5. Prerequisites

Install the following tools before running the application:

1. **Visual Studio Code** – Code editor.
2. **MSYS2 UCRT64 / MinGW-w64** – C++ compiler.
3. **CMake 3.20 or later** – Build configuration.
4. **Ninja** – Build tool.
5. **MySQL Server** – Database management.
6. **MariaDB Connector/C** – MySQL-compatible database connector.
7. **Git** – Version control.

The Crow framework is included in the project's `backend/third_party/Crow` directory.

Ensure that the compiler and required dependencies are correctly configured in your system environment.

---

## 6. Installation and Setup

### Step 1: Clone the Repository

Open PowerShell or Git Bash and execute:

```bash
git clone https://github.com/RaviRoushan26/YatraRail.git
```

Navigate into the project directory:

```bash
cd YatraRail
```

### Step 2: Configure the Database

YatraRail uses MySQL for storing railway reservation data. The backend connects to the database using the MariaDB Connector/C library.

#### 2.1 Start MySQL Server

Ensure that MySQL Server is installed and running on your system.

The application uses the following database configuration:

| Configuration | Value |
|---|---|
| Host | `127.0.0.1` |
| Port | `3306` |
| Database | `railway_reservation` |
| Username | `DB_USER` environment variable |
| Password | `DB_PASSWORD` environment variable |

#### 2.2 Create the Database

Open MySQL Command Line Client or MySQL Workbench and execute:

```sql
CREATE DATABASE railway_reservation;
```

#### 2.3 Configure Database Credentials

The backend retrieves database credentials through Windows environment variables.

Open PowerShell and execute the following commands, replacing the placeholder values with your own MySQL credentials:

```powershell
$env:DB_USER = "your_mysql_username"
$env:DB_PASSWORD = "your_mysql_password"
```

These commands configure the variables for the current PowerShell session.

**Important:** Set these environment variables in the same terminal session from which you start the backend server.

#### 2.4 Create the Required Tables

Create all tables required by the application before running the backend.

The current `database/railway_schema.sql` file only selects the `railway_reservation` database and displays existing tables. It does not contain the `CREATE TABLE` statements.

#### 2.5 Configure Admin Credentials

The application uses environment variables to authenticate the administrator.

Configure the following variables in PowerShell:

```powershell
$env:RAILBOOK_ADMIN_USERNAME = "your_admin_username"
$env:RAILBOOK_ADMIN_PASSWORD = "your_secure_admin_password"
```

**Admin password requirements:**
- The password must contain at least 12 characters.
- Both environment variables must be configured before starting the backend.
- The credentials are read by the backend during admin authentication.

#### 2.6 Security Recommendations

- Never hardcode database or administrator passwords in source code.
- Never commit actual credentials to GitHub.
- Use secure passwords for database and administrator accounts.
- Configure environment variables in the same terminal session used to start the backend.
### Step 3: Configure the Backend

The backend uses C++17, Crow, CMake and Ninja.

From the root directory of the project, configure the backend:

```powershell
cmake -S backend -B backend/build -G Ninja
```

This command configures the CMake build system and generates Ninja build files.

### Step 4: Build the Application

After successful configuration, compile the backend:

```powershell
cmake --build backend/build
```

This builds the backend executable named:

```text
railway_server.exe
```

### Step 5: Run the Backend Server

Execute the following command from the project root directory using PowerShell:

```powershell
.\backend\build\railway_server.exe
```

Once successfully started, the Crow backend server listens on port `18080`.

The backend API is available at:

`http://localhost:18080`

The frontend's current CORS configuration allows requests from:

`http://localhost:5500`

To stop the server, press:

```text
Ctrl + C
```

### Important Notes

- Ensure that MySQL Server is running before starting the backend.
- Configure the database connection environment variables required by the application.
- Ensure that all required database tables exist.
- Make sure CMake, Ninja, GCC, Crow and MariaDB Connector/C are installed and configured correctly.

---

## 7. Application Modules

### Passenger Operations

The passenger module supports registration, authentication, train search, ticket booking, PNR lookup and cancellation.

### Administrative Operations

The administrative module provides authentication and access to the admin dashboard for managing railway operations.

---

## 8. Testing

The application has been tested locally for the following functionalities:

- Passenger registration and login
- Passenger logout
- Admin login and logout
- Train search
- Ticket booking
- PNR lookup
- Ticket cancellation
- Backend server execution

---

## 9. Screenshots

Screenshots of the application interface will be added here.

Planned screenshots:

- Passenger registration and login page
- Train search interface
- Train search results
- Ticket booking interface
- PNR lookup
- Admin dashboard

---

## 10. Future Enhancements

- Deployment of the application for public access.
- Improved frontend design and user experience.
- Additional railway reservation functionalities.
- Enhanced database management.
- Additional testing and performance improvements.

---

## 11. Developer Information

**Project Name:** YatraRail – Railway Reservation Web Application

**Institution:** JSS Science and Technology University (JSSSTU), Mysuru

**Department:** Information Science and Engineering

**Programming Language:** C++

**GitHub Repository:** https://github.com/RaviRoushan26/YatraRail

---

## 12. Disclaimer

This project is developed for academic and educational purposes. It is not an official IRCTC application and does not claim to provide live IRCTC seat availability or official railway booking integration.