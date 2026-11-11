// Usage: keygen <output-file>
// Writes a new keypair to the file and prints the account ID.
#include <iostream>

#include "crypto/keys.hpp"

int main(int argc, char** argv) {
  using namespace ledger::crypto;
  if (argc != 2) {
    std::cerr << "usage: keygen <output-file>\n";
    return 1;
  }
  try {
    KeyPair kp = generate_keypair();
    save_keypair(kp, argv[1]);
    std::cout << account_id(kp.pub) << "\n";
  } catch (const std::exception& e) {
    std::cerr << "keygen: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
