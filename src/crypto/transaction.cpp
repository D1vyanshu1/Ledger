#include "crypto/transaction.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "crypto/keys.hpp"

namespace ledger::crypto {

namespace {

void put_u64_be(Bytes& out, std::uint64_t v) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    out.push_back(static_cast<std::uint8_t>(v >> shift));
  }
}

std::uint64_t get_u64_be(const std::uint8_t* p) {
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
  return v;
}

void put_body(Bytes& out, const Transaction& tx) {
  out.insert(out.end(), tx.from.begin(), tx.from.end());
  out.insert(out.end(), tx.to.begin(), tx.to.end());
  put_u64_be(out, tx.amount);
  put_u64_be(out, tx.nonce);
}

}  // namespace

Bytes signing_bytes(const Transaction& tx) {
  Bytes out;
  out.reserve(kTxDomainTag.size() + kTxBodySize);
  out.insert(out.end(), kTxDomainTag.begin(), kTxDomainTag.end());
  put_body(out, tx);
  return out;
}

void sign_transaction(Transaction& tx, const SecretKey& sec) {
  if (public_key_of(sec) != tx.from) {
    throw std::invalid_argument("secret key does not belong to tx.from");
  }
  tx.sig = sign(sec, signing_bytes(tx));
}

bool verify_transaction(const Transaction& tx) {
  return verify(tx.from, signing_bytes(tx), tx.sig);
}

Bytes encode_transaction(const Transaction& tx) {
  Bytes out;
  out.reserve(kTxWireSize);
  put_body(out, tx);
  out.insert(out.end(), tx.sig.begin(), tx.sig.end());
  return out;
}

Transaction decode_transaction(const Bytes& wire) {
  if (wire.size() != kTxWireSize) {
    throw std::invalid_argument("transaction has wrong size: expected " +
                                std::to_string(kTxWireSize) + " bytes, got " +
                                std::to_string(wire.size()));
  }
  const std::uint8_t* p = wire.data();
  Transaction tx;
  std::copy(p, p + kPublicKeySize, tx.from.begin());  p += kPublicKeySize;
  std::copy(p, p + kPublicKeySize, tx.to.begin());    p += kPublicKeySize;
  tx.amount = get_u64_be(p);                          p += 8;
  tx.nonce = get_u64_be(p);                           p += 8;
  std::copy(p, p + kSignatureSize, tx.sig.begin());
  return tx;
}

Hash256 transaction_hash(const Transaction& tx) { return sha256(encode_transaction(tx)); }

}  // namespace ledger::crypto
