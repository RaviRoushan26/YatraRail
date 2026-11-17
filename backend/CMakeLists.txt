
cmake_minimum_required(VERSION 3.20)

project(RailwayReservationWeb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Crow web framework
add_subdirectory(third_party/Crow)

# Find the MySQL-compatible MariaDB connector
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED IMPORTED_TARGET libmariadb)

# Main server
add_executable(railway_server
    src/main.cpp
)

# Project header files
target_include_directories(railway_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link required libraries
target_link_libraries(railway_server PRIVATE
    Crow::Crow
    PkgConfig::MARIADB
    ws2_32
    mswsock
    bcrypt
)
