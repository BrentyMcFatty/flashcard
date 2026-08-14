cmake_minimum_required(VERSION 3.16)

project(GTKFlashcards C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

add_executable(flashcards
        main.c
)

target_link_libraries(flashcards
        PRIVATE
        PkgConfig::GTK4
        PkgConfig::SQLITE3
)