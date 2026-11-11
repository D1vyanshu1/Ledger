#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "crypto/hex.hpp"
#include "crypto/keys.hpp"
#include "crypto/transaction.hpp"

using namespace ledger::crypto;

namespace {

Transaction make_tx(const KeyPair& from, const KeyPair& to, std::uint64_t amount,
                    std::uint64_t nonce) {
  Transaction tx;
  tx.from = from.pub;
  tx.to = to.pub;
  tx.amount = amount;
  tx.nonce = nonce;
  return tx;
}

void expect_same(const Transaction& a, const Transaction& b) {
  EXPECT_EQ(a.from, b.from);
  EXPECT_EQ(a.to, b.to);
  EXPECT_EQ(a.amount, b.amount);
  EXPECT_EQ(a.nonce, b.nonce);
  EXPECT_EQ(a.sig, b.sig);
}

}  // namespace

// ---------- hex ----------

TEST(Hex, RoundTrip) {
  Bytes b{0x00, 0x01, 0xab, 0xff};
  EXPECT_EQ(to_hex(b), "0001abff");
  EXPECT_EQ(from_hex("0001ABff"), b);
}

TEST(Hex, RejectsBadInput) {
  EXPECT_THROW(from_hex("abc"), std::invalid_argument);         // odd length
  EXPECT_THROW(from_hex("zz"), std::invalid_argument);          // not hex
  EXPECT_THROW(from_hex_fixed<4>("0011"), std::invalid_argument);  // wrong length
}

// ---------- raw Ed25519 (proves the library is wired up correctly) ----------

// RFC 8032, section 7.1, TEST 1 (empty message).
TEST(Ed25519, Rfc8032TestVector1) {
  auto seed = from_hex_fixed<kSeedSize>(
      "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60");
  KeyPair kp = keypair_from_seed(seed);
  EXPECT_EQ(to_hex(kp.pub), "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");

  Bytes empty;
  Signature sig = sign(kp.sec, empty);
  EXPECT_EQ(to_hex(sig),
            "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bac"
            "c61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");
  EXPECT_TRUE(verify(kp.pub, empty, sig));
}

// ---------- keys ----------

TEST(Keys, AccountIdRoundTrip) {
  KeyPair kp = generate_keypair();
  std::string id = account_id(kp.pub);
  EXPECT_EQ(id.size(), 64u);
  EXPECT_EQ(parse_account_id(id), kp.pub);
}

TEST(Keys, FileRoundTripAndPermissions) {
  namespace fs = std::filesystem;
  fs::path path = fs::temp_directory_path() / "ledger_crypto_test.key";
  KeyPair kp = generate_keypair();
  save_keypair(kp, path.string());

  KeyPair loaded = load_keypair(path.string());
  EXPECT_EQ(loaded.pub, kp.pub);
  EXPECT_EQ(loaded.sec, kp.sec);

  auto perms = fs::status(path).permissions();
  EXPECT_EQ(perms & (fs::perms::group_all | fs::perms::others_all), fs::perms::none);
  fs::remove(path);
}

TEST(Keys, LoadRejectsMismatchedKeys) {
  namespace fs = std::filesystem;
  fs::path path = fs::temp_directory_path() / "ledger_crypto_bad.key";
  KeyPair a = generate_keypair();
  KeyPair b = generate_keypair();
  {
    std::ofstream out(path);
    out << "public " << to_hex(a.pub) << "\nsecret " << to_hex(b.sec) << "\n";
  }
  EXPECT_THROW(load_keypair(path.string()), std::runtime_error);
  fs::remove(path);
}

// ---------- transactions ----------

TEST(Transaction, SignThenVerify) {
  KeyPair alice = generate_keypair(), bob = generate_keypair();
  Transaction tx = make_tx(alice, bob, 500, 1);
  sign_transaction(tx, alice.sec);
  EXPECT_TRUE(verify_transaction(tx));
}

TEST(Transaction, UnsignedFailsVerify) {
  KeyPair alice = generate_keypair(), bob = generate_keypair();
  Transaction tx = make_tx(alice, bob, 500, 1);  // sig is all zeros
  EXPECT_FALSE(verify_transaction(tx));
}

TEST(Transaction, ChangingAnyFieldBreaksSignature) {
  KeyPair alice = generate_keypair(), bob = generate_keypair(), eve = generate_keypair();
  Transaction good = make_tx(alice, bob, 500, 1);
  sign_transaction(good, alice.sec);

  Transaction t = good; t.amount = 5000;   EXPECT_FALSE(verify_transaction(t));
  t = good;             t.nonce = 2;       EXPECT_FALSE(verify_transaction(t));
  t = good;             t.to = eve.pub;    EXPECT_FALSE(verify_transaction(t));
  t = good;             t.sig[0] ^= 0x01;  EXPECT_FALSE(verify_transaction(t));
}

TEST(Transaction, CannotSpendSomeoneElsesMoney) {
  KeyPair alice = generate_keypair(), bob = generate_keypair(), eve = generate_keypair();

  // Eve signs with her own key but claims the money comes from Alice.
  Transaction forged = make_tx(alice, eve, 1000, 1);
  forged.sig = sign(eve.sec, signing_bytes(forged));
  EXPECT_FALSE(verify_transaction(forged));

  // And the helper refuses outright.
  Transaction tx = make_tx(alice, bob, 1, 1);
  EXPECT_THROW(sign_transaction(tx, eve.sec), std::invalid_argument);
}

TEST(Transaction, EncodingLayoutIsFixed) {
  KeyPair alice = generate_keypair(), bob = generate_keypair();
  Transaction tx = make_tx(alice, bob, 0x0102030405060708ULL, 1);
  sign_transaction(tx, alice.sec);

  Bytes wire = encode_transaction(tx);
  ASSERT_EQ(wire.size(), kTxWireSize);
  EXPECT_TRUE(std::equal(alice.pub.begin(), alice.pub.end(), wire.begin()));
  EXPECT_TRUE(std::equal(bob.pub.begin(), bob.pub.end(), wire.begin() + 32));
  Bytes amount_be{1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_TRUE(std::equal(amount_be.begin(), amount_be.end(), wire.begin() + 64));
  Bytes nonce_be{0, 0, 0, 0, 0, 0, 0, 1};
  EXPECT_TRUE(std::equal(nonce_be.begin(), nonce_be.end(), wire.begin() + 72));
  EXPECT_TRUE(std::equal(tx.sig.begin(), tx.sig.end(), wire.begin() + 80));
}

TEST(Transaction, EncodeDecodeRoundTrip) {
  KeyPair alice = generate_keypair(), bob = generate_keypair();
  Transaction tx = make_tx(alice, bob, 123456789, 42);
  sign_transaction(tx, alice.sec);

  Transaction back = decode_transaction(encode_transaction(tx));
  expect_same(back, tx);
  EXPECT_TRUE(verify_transaction(back));
}

TEST(Transaction, DecodeRejectsWrongSize) {
  EXPECT_THROW(decode_transaction(Bytes(143)), std::invalid_argument);
  EXPECT_THROW(decode_transaction(Bytes(145)), std::invalid_argument);
  EXPECT_THROW(decode_transaction(Bytes{}), std::invalid_argument);
}

// Ed25519 is deterministic: same key + same tx -> same signature -> same hash.
// Every node will compute identical bytes, which is exactly what Raft needs.
TEST(Transaction, SigningIsDeterministic) {
  KeyPair alice = generate_keypair(), bob = generate_keypair();
  Transaction a = make_tx(alice, bob, 10, 1), b = make_tx(alice, bob, 10, 1);
  sign_transaction(a, alice.sec);
  sign_transaction(b, alice.sec);
  EXPECT_EQ(a.sig, b.sig);
  EXPECT_EQ(transaction_hash(a), transaction_hash(b));
}
