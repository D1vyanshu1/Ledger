#pragma once
// Basic byte types shared by the whole project.
// Fixed-size things (keys, signatures, hashes) are std::array so the
// compiler stops you from mixing them up or passing the wrong length.

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ledger::crypto {

constexpr std::size_t kPublicKeySize = 32;  // Ed25519 public key
constexpr std::size_t kSecretKeySize = 64;  // libsodium format: seed(32) || public key(32)
constexpr std::size_t kSeedSize      = 32;  // Ed25519 seed (the "real" private key)
constexpr std::size_t kSignatureSize = 64;  // Ed25519 signature
constexpr std::size_t kHashSize      = 32;  // SHA-256 digest

using Bytes     = std::vector<std::uint8_t>;
using PublicKey = std::array<std::uint8_t, kPublicKeySize>;
using SecretKey = std::array<std::uint8_t, kSecretKeySize>;
using Seed      = std::array<std::uint8_t, kSeedSize>;
using Signature = std::array<std::uint8_t, kSignatureSize>;
using Hash256   = std::array<std::uint8_t, kHashSize>;

}  // namespace ledger::crypto
