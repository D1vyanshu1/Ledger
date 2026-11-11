cmake_minimum_required(VERSION 3.16)
project(ledger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LEDGER_SANITIZE "Build with AddressSanitizer + UBSan" OFF)
if(LEDGER_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()
add_compile_options(-Wall -Wextra -Wpedantic)

# libsodium provides Ed25519 and SHA-256
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

# ---- Module 1: crypto ----
add_library(ledger_crypto
  src/crypto/hex.cpp
  src/crypto/keys.cpp
  src/crypto/transaction.cpp
)
target_include_directories(ledger_crypto PUBLIC src)
target_link_libraries(ledger_crypto PUBLIC PkgConfig::SODIUM)

add_executable(keygen src/apps/keygen.cpp)
target_link_libraries(keygen PRIVATE ledger_crypto)

# ---- Tests (GoogleTest, downloaded automatically) ----
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(crypto_test tests/unit/crypto_test.cpp)
target_link_libraries(crypto_test PRIVATE ledger_crypto GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(crypto_test)
