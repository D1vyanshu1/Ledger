#pragma once
// Keys, signing, verification, hashing. Thin wrappers over libsodium.

#include <string>
#include <string_view>

#include "crypto/types.hpp"

namespace ledger::crypto {

struct KeyPair {
  PublicKey pub{};
  SecretKey sec{};
};

// Initialises libsodium. Safe to call many times; every function below
// calls it for you, so you rarely need to call it yourself.
void init();

KeyPair generate_keypair();                    // random keypair
KeyPair keypair_from_seed(const Seed& seed);   // deterministic (tests, genesis accounts)
PublicKey public_key_of(const SecretKey& sec); // recover pub from sec

// Account ID = public key in lowercase hex (64 characters).
std::string account_id(const PublicKey& pub);
PublicKey parse_account_id(std::string_view id);

// Key file format (plain text, permissions set to 0600):
//   public <64 hex chars>
//   secret <128 hex chars>
void save_keypair(const KeyPair& kp, const std::string& path);
KeyPair load_keypair(const std::string& path);  // checks pub matches sec

Signature sign(const SecretKey& sec, const std::uint8_t* msg, std::size_t len);
bool verify(const PublicKey& pub, const std::uint8_t* msg, std::size_t len, const Signature& sig);

inline Signature sign(const SecretKey& sec, const Bytes& msg) {
  return sign(sec, msg.data(), msg.size());
}
inline bool verify(const PublicKey& pub, const Bytes& msg, const Signature& sig) {
  return verify(pub, msg.data(), msg.size(), sig);
}

Hash256 sha256(const std::uint8_t* data, std::size_t len);
inline Hash256 sha256(const Bytes& b) { return sha256(b.data(), b.size()); }

}  // namespace ledger::crypto
