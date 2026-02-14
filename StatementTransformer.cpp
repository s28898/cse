//
// Created by Bruno Kuś on 05/11/2025.
//
#include <string>
#include "StatementTransformer.h"
bsoncxx::types::b_binary binary_from_string(std::string ciphertext)
{
  // TODO!
  auto leak = malloc(ciphertext.size());
  memcpy(leak, ciphertext.data(), ciphertext.size());
  return bsoncxx::types::b_binary{
    bsoncxx::binary_sub_type::k_binary,
    static_cast<uint32_t>(ciphertext.size()),
    reinterpret_cast<const uint8_t *>(leak)
  };
}