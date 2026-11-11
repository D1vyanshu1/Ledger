#include "crypto/hex.hpp"

namespace ledger::crypto {

namespace {
int nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
}  // namespace

std::string to_hex(const std::uint8_t* data, std::size_t len) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (std::size_t i = 0; i < len; ++i) {
    out.push_back(kDigits[data[i] >> 4]);
    out.push_back(kDigits[data[i] & 0x0f]);
  }
  return out;
}

Bytes from_hex(std::string_view hex) {
  if (hex.size() % 2 != 0) throw std::invalid_argument("hex string has odd length");
  Bytes out(hex.size() / 2);
  for (std::size_t i = 0; i < out.size(); ++i) {
    int hi = nibble(hex[2 * i]);
    int lo = nibble(hex[2 * i + 1]);
    if (hi < 0 || lo < 0) throw std::invalid_argument("invalid hex character");
    out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return out;
}

}  // namespace ledger::crypto
