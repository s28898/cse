// EncryptionService_v3.h
#ifndef MONGO_2_ENCRYPTION_SERVICE_V3_H
#define MONGO_2_ENCRYPTION_SERVICE_V3_H

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>

#include <bsoncxx/types.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../ope/ope.hh"
#include "../AbstractKeyProvider.h"
#include "../EncryptionProfile.h"

using CryptoPP::AES;

struct EncryptionServiceV3
{
    explicit EncryptionServiceV3(std::shared_ptr<AbstractKeyProvider> provider)
      : m_provider(std::move(provider)) {}

    EncryptionServiceV3(const EncryptionServiceV3&) = delete;
    EncryptionServiceV3& operator=(const EncryptionServiceV3&) = delete;
    EncryptionServiceV3(EncryptionServiceV3&&) noexcept = delete;
    EncryptionServiceV3& operator=(EncryptionServiceV3&&) noexcept = delete;


    enum class AesKind
    {
        AES256_CTR,
        AES192_CTR
    };

    std::string encrypt_aes(std::string_view collection,
                            std::string_view property_path,
                            AesKind kind,
                            std::string_view plaintext)
    {
      const auto profile = to_profile(kind);
      const auto km = get_cached_key(collection, profile);
      validate_aes_key(kind, km.bytes);

      const std::string ctx = make_context(collection, property_path, kind);
      const auto iv = derive_iv_16_hmac_sha256(km.bytes, ctx, plaintext);

      CryptoPP::CTR_Mode<AES>::Encryption enc;
      enc.SetKeyWithIV(km.bytes.data(),
                       required_key_len(kind),
                       iv.data(),
                       iv.size());

      std::string ciphertext;
      ciphertext.reserve(plaintext.size());

      CryptoPP::StringSource ss(
        reinterpret_cast<const CryptoPP::byte*>(plaintext.data()),
        plaintext.size(),
        true,
        new CryptoPP::StreamTransformationFilter(
          enc,
          new CryptoPP::StringSink(ciphertext)));

      std::string out;
      out.reserve(iv.size() + ciphertext.size());
      out.append(reinterpret_cast<const char*>(iv.data()), iv.size());
      out.append(ciphertext);
      return out;
    }

    std::string decrypt_aes(std::string_view collection,
                            AesKind kind,
                            const bsoncxx::types::b_binary& ciphertext_blob)
    {
      const auto profile = to_profile(kind);
      const auto km = get_cached_key(collection, profile);
      validate_aes_key(kind, km.bytes);

      const auto total = static_cast<std::size_t>(ciphertext_blob.size);
      if (total < k_iv_len)
        throw std::runtime_error("AES-CTR ciphertext too short");

      const auto* bytes = reinterpret_cast<const std::uint8_t*>(ciphertext_blob.bytes);

      std::array<std::uint8_t, k_iv_len> iv{};
      std::memcpy(iv.data(), bytes, iv.size());

      const auto* cptr = bytes + iv.size();
      const auto clen = total - iv.size();

      CryptoPP::CTR_Mode<AES>::Decryption dec;
      dec.SetKeyWithIV(km.bytes.data(),
                       required_key_len(kind),
                       iv.data(),
                       iv.size());

      std::string plaintext;
      plaintext.reserve(clen);

      CryptoPP::StringSource ss(
        cptr,
        clen,
        true,
        new CryptoPP::StreamTransformationFilter(
          dec,
          new CryptoPP::StringSink(plaintext)));

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

    std::string decrypt_aes(std::string_view collection,
                            std::string_view property_path,
                            AesKind kind,
                            const bsoncxx::types::b_binary& ciphertext_blob)
    {
      const auto profile = to_profile(kind);
      const auto km = get_cached_key(collection, profile);
      validate_aes_key(kind, km.bytes);

      const auto total = static_cast<std::size_t>(ciphertext_blob.size);
      if (total < k_iv_len)
        throw std::runtime_error("AES-CTR ciphertext too short");

      const auto* bytes = reinterpret_cast<const std::uint8_t*>(ciphertext_blob.bytes);

      std::array<std::uint8_t, k_iv_len> iv{};
      std::memcpy(iv.data(), bytes, iv.size());

      const auto* cptr = bytes + iv.size();
      const auto clen = total - iv.size();

      CryptoPP::CTR_Mode<AES>::Decryption dec;
      dec.SetKeyWithIV(km.bytes.data(),
                       required_key_len(kind),
                       iv.data(),
                       iv.size());

      std::string plaintext;
      plaintext.reserve(clen);

      CryptoPP::StringSource ss(
        cptr,
        clen,
        true,
        new CryptoPP::StreamTransformationFilter(
          dec,
          new CryptoPP::StringSink(plaintext)));

      const std::string ctx = make_context(collection, property_path, kind);
      const auto iv2 = derive_iv_16_hmac_sha256(km.bytes, ctx, plaintext);

      if (!std::equal(iv.begin(), iv.end(), iv2.begin()))
      {
        throw std::runtime_error(
          "decrypt_aes: IV verification failed (wrong key/context or wrong field). "
          "collection=" + std::string(collection) +
          " property_path=" + std::string(property_path)
                                );
      }

      return plaintext;
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
    static constexpr std::size_t k_iv_len = 16;

    std::shared_ptr<AbstractKeyProvider> m_provider;

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
          const std::size_t h1 = std::hash<std::string>{}(k.collection);
          const std::size_t h2 = static_cast<std::size_t>(k.profile);
          return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };

    std::unordered_map<CacheKey, AbstractKeyProvider::KeyMaterial, CacheKeyHash> m_cache;

    static EncryptionProfile to_profile(AesKind kind)
    {
      return (kind == AesKind::AES256_CTR)
             ? EncryptionProfile::AES256_CTR
             : EncryptionProfile::AES192_CTR;
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

    static std::size_t required_key_len(AesKind kind)
    {
      return (kind == AesKind::AES256_CTR) ? 32 : 24;
    }

    static void validate_aes_key(AesKind kind, const std::vector<std::uint8_t>& bytes)
    {
      const std::size_t need = required_key_len(kind);
      if (bytes.size() != need)
        throw std::runtime_error("AES key has invalid length for selected profile");
    }

    static std::string make_context(std::string_view collection,
                                    std::string_view property_path,
                                    AesKind kind)
    {
      std::string ctx;
      ctx.reserve(collection.size() + 1 + property_path.size() + 16);
      ctx.append(collection);
      ctx.push_back('/');
      ctx.append(property_path);
      ctx.append(kind == AesKind::AES256_CTR ? "#AES256_CTR" : "#AES192_CTR");
      return ctx;
    }

    static std::array<std::uint8_t, k_iv_len> derive_iv_16_hmac_sha256(
      const std::vector<std::uint8_t>& key_bytes,
      std::string_view context,
      std::string_view plaintext)
    {
      std::string msg;
      msg.reserve(context.size() + 1 + plaintext.size());
      msg.append(context);
      msg.push_back('\0');
      msg.append(plaintext);

      CryptoPP::HMAC<CryptoPP::SHA256> hmac(key_bytes.data(), key_bytes.size());

      std::uint8_t digest[CryptoPP::SHA256::DIGESTSIZE];
      hmac.CalculateDigest(digest,
                           reinterpret_cast<const CryptoPP::byte*>(msg.data()),
                           msg.size());

      std::array<std::uint8_t, k_iv_len> iv{};
      std::memcpy(iv.data(), digest, iv.size());
      return iv;
    }

    static std::string key_bytes_as_string(const std::vector<std::uint8_t>& bytes)
    {
      return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
};

#endif // MONGO_2_ENCRYPTION_SERVICE_V3_H
