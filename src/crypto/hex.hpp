#pragma once
// Hex encoding: used for account IDs, key files, and debug output.

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

#include "crypto/types.hpp"

namespace ledger::crypto {

std::string to_hex(const std::uint8_t* data, std::size_t len);

inline std::string to_hex(const Bytes& b) { return to_hex(b.data(), b.size()); }

template <std::size_t N>
std::string to_hex(const std::array<std::uint8_t, N>& a) {
  return to_hex(a.data(), N);
}

// Accepts upper or lower case. Throws std::invalid_argument on bad input.
Bytes from_hex(std::string_view hex);

// Same, but also checks the decoded length is exactly N bytes.
template <std::size_t N>
std::array<std::uint8_t, N> from_hex_fixed(std::string_view hex) {
  Bytes b = from_hex(hex);
  if (b.size() != N) {
    throw std::invalid_argument("hex has wrong length: expected " + std::to_string(N) +
                                " bytes, got " + std::to_string(b.size()));
  }
  std::array<std::uint8_t, N> out{};
  std::copy(b.begin(), b.end(), out.begin());
  return out;
}

}  // namespace ledger::crypto
