cmake_minimum_required(VERSION 3.15)
project(polynomial_dependency_checker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GINAC REQUIRED ginac)
pkg_check_modules(CLN REQUIRED cln)

find_package(SQLite3 REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(poly_dep_check
    src/main.cpp
    src/config.cpp
    src/symbols.cpp
    src/polynomial.cpp
    src/generator.cpp
    src/dependency_finder.cpp
    src/divisibility.cpp
    src/cache.cpp
    src/state.cpp
    src/brute_force.cpp
    src/manual_check.cpp
    src/cli.cpp
)

target_include_directories(poly_dep_check PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GINAC_INCLUDE_DIRS}
    ${CLN_INCLUDE_DIRS}
)

target_link_libraries(poly_dep_check PRIVATE
    ${GINAC_LIBRARIES}
    ${CLN_LIBRARIES}
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    pthread
)

target_compile_options(poly_dep_check PRIVATE
    -Wall -Wextra -Wpedantic
    -fcoroutines
    ${GINAC_CFLAGS_OTHER}
    ${CLN_CFLAGS_OTHER}
)

install(TARGETS poly_dep_check DESTINATION bin)