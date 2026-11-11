#include "crypto/keys.hpp"

#include <sodium.h>

#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>

#include "crypto/hex.hpp"

namespace ledger::crypto {

// If any of these fail, our sizes in types.hpp disagree with libsodium.
static_assert(kPublicKeySize == crypto_sign_PUBLICKEYBYTES);
static_assert(kSecretKeySize == crypto_sign_SECRETKEYBYTES);
static_assert(kSeedSize == crypto_sign_SEEDBYTES);
static_assert(kSignatureSize == crypto_sign_BYTES);
static_assert(kHashSize == crypto_hash_sha256_BYTES);

void init() {
  static std::once_flag flag;
  std::call_once(flag, [] {
    if (sodium_init() < 0) throw std::runtime_error("libsodium initialisation failed");
  });
}

KeyPair generate_keypair() {
  init();
  KeyPair kp;
  crypto_sign_keypair(kp.pub.data(), kp.sec.data());
  return kp;
}

KeyPair keypair_from_seed(const Seed& seed) {
  init();
  KeyPair kp;
  crypto_sign_seed_keypair(kp.pub.data(), kp.sec.data(), seed.data());
  return kp;
}

PublicKey public_key_of(const SecretKey& sec) {
  init();
  PublicKey pub;
  crypto_sign_ed25519_sk_to_pk(pub.data(), sec.data());
  return pub;
}

std::string account_id(const PublicKey& pub) { return to_hex(pub); }

PublicKey parse_account_id(std::string_view id) { return from_hex_fixed<kPublicKeySize>(id); }

void save_keypair(const KeyPair& kp, const std::string& path) {
  {
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw std::runtime_error("cannot open key file for writing: " + path);
    out << "public " << to_hex(kp.pub) << "\n"
        << "secret " << to_hex(kp.sec) << "\n";
    if (!out) throw std::runtime_error("failed writing key file: " + path);
  }
  namespace fs = std::filesystem;
  fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write,
                  fs::perm_options::replace);
}

KeyPair load_keypair(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open key file: " + path);

  std::string label_pub, hex_pub, label_sec, hex_sec;
  if (!(in >> label_pub >> hex_pub >> label_sec >> hex_sec) || label_pub != "public" ||
      label_sec != "secret") {
    throw std::runtime_error("malformed key file: " + path);
  }

  KeyPair kp;
  kp.pub = from_hex_fixed<kPublicKeySize>(hex_pub);
  kp.sec = from_hex_fixed<kSecretKeySize>(hex_sec);
  if (public_key_of(kp.sec) != kp.pub) {
    throw std::runtime_error("key file is inconsistent (public key does not match secret): " +
                             path);
  }
  return kp;
}

Signature sign(const SecretKey& sec, const std::uint8_t* msg, std::size_t len) {
  init();
  Signature sig;
  crypto_sign_detached(sig.data(), nullptr, msg, len, sec.data());
  return sig;
}

bool verify(const PublicKey& pub, const std::uint8_t* msg, std::size_t len, const Signature& sig) {
  init();
  return crypto_sign_verify_detached(sig.data(), msg, len, pub.data()) == 0;
}

Hash256 sha256(const std::uint8_t* data, std::size_t len) {
  init();
  Hash256 h;
  crypto_hash_sha256(h.data(), data, len);
  return h;
}

}  // namespace ledger::crypto
