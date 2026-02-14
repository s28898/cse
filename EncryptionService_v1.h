//
// Created by Bruno Kuś on 05/11/2025.
//

#ifndef MONGO_1_ENCRYPTION_AGENT_H
#define MONGO_1_ENCRYPTION_AGENT_H

#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <iostream>

#include <bsoncxx/builder/stream/document.hpp>
#include <string>

#include "ope/ope.hh"
//#include "

#include <fstream>

using namespace CryptoPP;

struct EncryptionService
{
    SecByteBlock key;
    byte iv[AES::BLOCKSIZE];
    
    ope::OPE ope_module{"my key", 32, 64};
    
    void load(const std::string &filename)
    {
      std::ifstream in(filename, std::ios::binary);
      if (!in) throw std::runtime_error("");
      
      size_t key_len;
      in.read(reinterpret_cast<char *>(&key_len), sizeof(size_t));
      
      key = SecByteBlock(key_len);
      in.read(reinterpret_cast<char *>(key.data()), key_len);
      in.read(reinterpret_cast<char *>(iv), AES::BLOCKSIZE);
      
      
      in.close();
      
    }
    
    
    std::uint64_t encrypt_int32(std::int32_t number)
    {
      if (number < 0) throw std::runtime_error("[EncryptionService]");
      static_assert(std::is_same_v<int, std::int32_t>);
      
      auto ZZ = ope_module.encrypt(number);
      auto ciphertext = ope::uint64FromZZ(std::move(ZZ));
      return ciphertext;
    }
    
    std::int32_t decrypt_uint64(std::uint64_t ciphertext)
    {
      auto ZZ = ope::ZZFromUint64(ciphertext);
      auto plainZZ = ope_module.decrypt(ZZ);
      auto result = NTL::to_int(plainZZ);
      static_assert(std::is_same_v<decltype(result), std::int32_t>);
      return result;
    }
    
    
    template<class T>
    std::string encrypt_AES(const T number)requires std::is_arithmetic_v<T>
    {
      SecByteBlock plain(sizeof(T));
      std::memcpy(plain.BytePtr(), &number, sizeof(T));
      
      CBC_Mode<AES>::Encryption enc;
      enc.SetKeyWithIV(key, key.size(), iv);
      
      std::string cipher;
      ArraySource(
        plain, plain.size(), true,
        new StreamTransformationFilter(enc, new StringSink(cipher))
                 );
      return cipher;
    }
    
    std::string encrypt_AES(const std::string &plaintext)
    {
      std::string ciphertext;
      
      CBC_Mode<AES>::Encryption encryption;
      encryption.SetKeyWithIV(key, key.size(), iv);
      
      StringSource ss(
        plaintext, true,
        new StreamTransformationFilter(
          encryption,
          new StringSink(ciphertext)
                                      )
                     );
      
      return ciphertext;
    }
    
    std::string decrypt_AES(const bsoncxx::types::b_binary &ciphertext)
    {
      std::string decrypted;
      
      CBC_Mode<AES>::Decryption decryption;
      decryption.SetKeyWithIV(key, key.size(), iv);
      
      StringSource ss(
        ciphertext.bytes, ciphertext.size, true,
        new StreamTransformationFilter(
          decryption,
          new StringSink(decrypted)
                                      )
                     );

//        std::cout << "decrypt_AES :: decrypted == " << decrypted << "\n";
      return decrypted;
    }
    
    std::string decrypt_AES(const std::string &ciphertext) // helper
    {
      std::string decrypted;
      
      CBC_Mode<AES>::Decryption decryption;
      decryption.SetKeyWithIV(key, key.size(), iv);
      
      StringSource ss(
        ciphertext, true,
        new StreamTransformationFilter(
          decryption,
          new StringSink(decrypted)
                                      )
                     );

//      std::cout << "DECRYPTED : " << decrypted << "\n";
      return decrypted;
    }

//    int64_t decrypt_OPE()
    
    static void generate_context(const std::string &filename)
    {
      AutoSeededRandomPool pool;
      SecByteBlock key(AES::MAX_KEYLENGTH);
      pool.GenerateBlock(key, key.size());
      
      byte iv[AES::BLOCKSIZE];
      pool.GenerateBlock(iv, sizeof(iv));
      std::ofstream out(filename, std::ios::binary);
      
      size_t key_len = key.size();
      out.write(reinterpret_cast<const char *>(&key_len), sizeof(size_t));
      out.write(reinterpret_cast<const char *>(key.data()), key_len);
      
      // Write IV
      out.write(reinterpret_cast<const char *>(iv), AES::BLOCKSIZE);
      
      out.close();
    }
};


#endif //MONGO_1_ENCRYPTION_AGENT_H
