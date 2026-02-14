
#ifndef MONGO_2_ENCRYPTION_SERVICE_H
#define MONGO_2_ENCRYPTION_SERVICE_H

#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>

#include <bsoncxx/types.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include "../ope/ope.hh"
#include "../AbstractKeyProvider.h"
#include "../EncryptionProfile.h"

using CryptoPP::AES;
using CryptoPP::GCM;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::AuthenticatedEncryptionFilter;
using CryptoPP::AuthenticatedDecryptionFilter;
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::byte;



struct EncryptionServiceV2
{
    explicit EncryptionServiceV2(std::shared_ptr<AbstractKeyProvider> provider)
      : m_provider(std::move(provider)) {}
    
    EncryptionServiceV2(const EncryptionServiceV2&) = delete;
    EncryptionServiceV2& operator=(const EncryptionServiceV2&) = delete;
    EncryptionServiceV2(EncryptionServiceV2&&) noexcept = delete;
    
    
    enum class AesKind
    {
        AES256_GCM,
        AES192_GCM
    };
    
    
    
    std::string encrypt_aes(std::string_view collection, AesKind kind, std::string_view plaintext)
    {
      const auto km = get_cached_key(collection, to_profile(kind));
      validate_aes_key(kind, km.bytes);
      
      std::string nonce(12, '\0');
      m_rng.GenerateBlock(reinterpret_cast<byte*>(nonce.data()), nonce.size());
      
      std::string cipher_and_tag;
      cipher_and_tag.reserve(plaintext.size() + 16);
      
      if (kind == AesKind::AES256_GCM)
        gcm_encrypt<AES::DEFAULT_KEYLENGTH>(km, nonce, plaintext, cipher_and_tag);
      else
        gcm_encrypt<24 /* 192 bits */>(km, nonce, plaintext, cipher_and_tag);
      
   
      std::string out;
      out.reserve(nonce.size() + cipher_and_tag.size());
      out.append(nonce);
      out.append(cipher_and_tag);
      return out;
    }
    
    std::string decrypt_aes(std::string_view collection, AesKind kind, const bsoncxx::types::b_binary& ciphertext)
    {
      const auto km = get_cached_key(collection, to_profile(kind));
      validate_aes_key(kind, km.bytes);
      
      const auto total = static_cast<std::size_t>(ciphertext.size);
      if (total < 12 + 16)
        throw std::runtime_error("AES-GCM ciphertext too short");
      
      const auto* bytes = reinterpret_cast<const char*>(ciphertext.bytes);
      const std::string nonce(bytes, bytes + 12);
      const std::string cipher_and_tag(bytes + 12, bytes + total);
      
      std::string plaintext;
      
      if (kind == AesKind::AES256_GCM)
        gcm_decrypt<AES::DEFAULT_KEYLENGTH>(km, nonce, cipher_and_tag, plaintext);
      else
        gcm_decrypt<24>(km, nonce, cipher_and_tag, plaintext);
      
      return plaintext;
    }
    
    
    std::uint64_t encrypt_ope_int32(std::string_view collection, std::int32_t value)
    {
      if (value < 0)
        throw std::runtime_error("OPE_32_64 supports only non-negative int32");
      
      const auto km = get_cached_key(collection, EncryptionProfile::OPE_32_64);
      const auto key_str = key_bytes_as_string(km.bytes);
      
      ope::OPE ope_module{key_str, 32, 64};
      auto zz = ope_module.encrypt(static_cast<std::uint32_t>(value));
      return ope::uint64FromZZ(std::move(zz));
    }
    
    std::int32_t decrypt_ope_uint64(std::string_view collection, std::uint64_t ciphertext)
    {
      const auto km = get_cached_key(collection, EncryptionProfile::OPE_32_64);
      const auto key_str = key_bytes_as_string(km.bytes);
      
      ope::OPE ope_module{key_str, 32, 64};
      auto zz = ope::ZZFromUint64(ciphertext);
      auto plainZZ = ope_module.decrypt(zz);
      
      const int plain = NTL::to_int(plainZZ);
      if (plain < 0)
        throw std::runtime_error("OPE decrypted to negative value (unexpected for our domain)");
      
      return static_cast<std::int32_t>(plain);
    }
    
    private:
    std::shared_ptr<AbstractKeyProvider> m_provider;
    AutoSeededRandomPool m_rng;
    
    struct CacheKey
    {
        std::string collection;
        EncryptionProfile profile;
        
        bool operator==(const CacheKey& other) const
        {
          return collection == other.collection && profile == other.profile;
        }
    };
    
    struct CacheKeyHash
    {
        std::size_t operator()(const CacheKey& k) const noexcept
        {
          return std::hash<std::string>{}(k.collection) ^ (static_cast<std::size_t>(k.profile) << 1);
        }
    };
    
    std::unordered_map<CacheKey, AbstractKeyProvider::KeyMaterial, CacheKeyHash> m_cache;
    
    static EncryptionProfile to_profile(AesKind kind)
    {
      return (kind == AesKind::AES256_GCM)
             ? EncryptionProfile::AES256_GCM
             : EncryptionProfile::AES192_GCM;
    }
    
    AbstractKeyProvider::KeyMaterial get_cached_key(std::string_view collection, EncryptionProfile profile)
    {
      CacheKey ck{std::string(collection), profile};
      if (auto it = m_cache.find(ck); it != m_cache.end())
        return it->second;
      
      auto km = m_provider->get_active_key(collection, profile);
      m_cache.emplace(std::move(ck), km);
      return km;
    }
    
    static void validate_aes_key(AesKind kind, const std::vector<std::uint8_t>& bytes)
    {
      const std::size_t need = (kind == AesKind::AES256_GCM) ? 32 : 24;
      if (bytes.size() != need)
        throw std::runtime_error("AES key has invalid length for selected profile");
    }
    
    static std::string key_bytes_as_string(const std::vector<std::uint8_t>& bytes)
    {
      return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    
    template<std::size_t KeyLen>
    static void gcm_encrypt(const AbstractKeyProvider::KeyMaterial& km,
                            const std::string& nonce,
                            std::string_view plaintext,
                            std::string& out_cipher_and_tag)
    {
      GCM<AES>::Encryption enc;
      enc.SetKeyWithIV(km.bytes.data(), KeyLen,
                       reinterpret_cast<const byte*>(nonce.data()), nonce.size());
      
      CryptoPP::StringSource ss(
        reinterpret_cast<const CryptoPP::byte*>(plaintext.data()),
        plaintext.size(),
        true,
        new AuthenticatedEncryptionFilter(
          enc,
          new StringSink(out_cipher_and_tag),
          false,
          16 // 16-byte tag
        )
      );
    }
    
    template<std::size_t KeyLen>
    static void gcm_decrypt(const AbstractKeyProvider::KeyMaterial& km,
                            const std::string& nonce,
                            const std::string& cipher_and_tag,
                            std::string& out_plaintext)
    {
      GCM<AES>::Decryption dec;
      dec.SetKeyWithIV(km.bytes.data(), KeyLen,
                       reinterpret_cast<const byte*>(nonce.data()), nonce.size());
      
      AuthenticatedDecryptionFilter df(dec, new StringSink(out_plaintext),
                                       AuthenticatedDecryptionFilter::DEFAULT_FLAGS, 16);
      
      CryptoPP::StringSource ss(
        cipher_and_tag, true,
        new CryptoPP::Redirector(df)
                               );
      
      if (!df.GetLastResult())
        throw std::runtime_error("AES-GCM authentication failed (wrong key or corrupted ciphertext)");
    }
};
#endif // MONGO_2_ENCRYPTION_SERVICE_H