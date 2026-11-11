#pragma once
// Transaction struct + its canonical byte encoding.
//
// Wire layout (fixed 144 bytes, all integers big-endian):
//   offset  0: from      (32 bytes)
//   offset 32: to        (32 bytes)
//   offset 64: amount    (uint64, 8 bytes)  -- integer paise, never floats
//   offset 72: nonce     (uint64, 8 bytes)
//   offset 80: signature (64 bytes)
//
// The signature covers  kTxDomainTag || bytes[0..80)
// The domain tag stops a signature made for something else from being
// reused as a transaction signature.

#include <cstdint>
#include <string_view>

#include "crypto/types.hpp"

namespace ledger::crypto {

constexpr std::size_t kTxBodySize = 2 * kPublicKeySize + 8 + 8;    // 80
constexpr std::size_t kTxWireSize = kTxBodySize + kSignatureSize;  // 144
inline constexpr std::string_view kTxDomainTag = "ledger-tx-v1";

struct Transaction {
  PublicKey from{};
  PublicKey to{};
  std::uint64_t amount = 0;
  std::uint64_t nonce = 0;
  Signature sig{};
};

// The exact bytes that get signed (domain tag + body, no signature).
Bytes signing_bytes(const Transaction& tx);

// Fills tx.sig. Throws if `sec` is not the secret key of tx.from.
void sign_transaction(Transaction& tx, const SecretKey& sec);

// True iff tx.sig is a valid signature by tx.from over signing_bytes(tx).
// Does NOT check balance or nonce -- that is the ledger's job (module 2).
bool verify_transaction(const Transaction& tx);

// Full encoding including signature. This is what goes into the Raft log.
Bytes encode_transaction(const Transaction& tx);
Transaction decode_transaction(const Bytes& wire);  // throws on wrong size

// SHA-256 of the full encoding. Useful later for idempotent retries.
Hash256 transaction_hash(const Transaction& tx);

}  // namespace ledger::crypto
