cmake_minimum_required(VERSION 3.16)
project(kasturisundari-chain VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Options ─────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" ON)

# ─── Third-party dependencies ────────────────────────────────────────
add_subdirectory(third_party)

# ─── Core library ────────────────────────────────────────────────────
add_subdirectory(src)

# ─── Applications ────────────────────────────────────────────────────
add_subdirectory(apps)

# ─── Tests ───────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
