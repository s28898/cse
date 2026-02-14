//
// Created by Bruno Kuś on 08/02/2026.
// Updated for CTR-only profiles on 12/02/2026.
//

#ifndef MONGO_2_DUMMYKEYPROVIDER_H
#define MONGO_2_DUMMYKEYPROVIDER_H

#include <cryptopp/osrng.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

#include "AbstractKeyProvider.h"
#include "KeyPolicy.h"



class DummyKeyProvider final : public AbstractKeyProvider
{
    public:
    DummyKeyProvider() = default;
    
    DummyKeyProvider(std::shared_ptr<DatabaseKeyPolicy> policy, std::filesystem::path root_dir)
      : m_policy(std::move(policy))
      , m_root_dir(std::move(root_dir))
    {
      ensure_initialized();
    }
    
    KeyMaterial get_active_key(std::string_view collection, EncryptionProfile profile) override
    {
      ensure_initialized();
      
      validate_supported_profile(profile);
      
      const auto collPolicy = m_policy->collectionKeyPolicy(collection);
      const auto key_id = collPolicy.keyId(profile);
      
      const auto path = resolve_path(key_id);
      auto bytes = read_all_bytes(path);
      
      validate_length(profile, bytes, path);
      
      KeyMaterial km;
      km.profile = profile;
      km.bytes = std::move(bytes);
      return km;
    }
    
    KeyMaterial get_key(std::string_view collection, EncryptionProfile profile, std::string_view version) override
    {
      (void)version;
      return get_active_key(collection, profile);
    }
    
    [[nodiscard]] const DatabaseKeyPolicy& policy() const
    {
      ensure_initialized();
      return *m_policy;
    }
    
    [[nodiscard]] const std::filesystem::path& root_dir() const
    {
      ensure_initialized();
      return m_root_dir;
    }
    
    private:
    std::shared_ptr<DatabaseKeyPolicy> m_policy{};
    std::filesystem::path m_root_dir{};
    
    void ensure_initialized() const
    {
      if (!m_policy || m_policy->empty())
        throw std::runtime_error("DummyKeyProvider: policy is empty / not initialized");
      if (m_root_dir.empty())
        throw std::runtime_error("DummyKeyProvider: root_dir is empty / not initialized");
    }
    
    static void validate_supported_profile(EncryptionProfile profile)
    {
      switch (profile)
      {
        case EncryptionProfile::AES256_CTR:
        case EncryptionProfile::AES192_CTR:
        case EncryptionProfile::OPE_32_64:
          return;
        default:
          throw std::runtime_error("DummyKeyProvider: unsupported EncryptionProfile (expected CTR-only + OPE_32_64)");
      }
    }
    
    std::filesystem::path resolve_path(std::string_view key_id) const
    {
      std::filesystem::path rel{std::string(key_id)};
      
      if (rel.is_absolute())
        throw std::runtime_error("DummyKeyProvider: keyId must be relative, got absolute path: " + rel.string());
      
      return m_root_dir / rel;
    }
    
    static std::vector<std::uint8_t> read_all_bytes(const std::filesystem::path& p)
    {
      std::ifstream in(p, std::ios::binary);
      if (!in)
        throw std::runtime_error("DummyKeyProvider: cannot open key file: " + p.string());
      
      in.seekg(0, std::ios::end);
      const auto size_ll = in.tellg();
      if (size_ll < 0)
        throw std::runtime_error("DummyKeyProvider: tellg() failed for key file: " + p.string());
      
      const auto size = static_cast<std::size_t>(size_ll);
      in.seekg(0, std::ios::beg);
      
      std::vector<std::uint8_t> buf(size);
      if (size != 0)
        in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
      
      if (!in && size != 0)
        throw std::runtime_error("DummyKeyProvider: failed to read key file: " + p.string());
      
      return buf;
    }
    
    static void validate_length(EncryptionProfile profile,
                                const std::vector<std::uint8_t>& bytes,
                                const std::filesystem::path& p)
    {
      const std::size_t need =
        (profile == EncryptionProfile::AES256_CTR) ? 32 :
        (profile == EncryptionProfile::AES192_CTR) ? 24 :
        32; // OPE_32_64 fixed-length key for PoC
      
      if (bytes.size() != need)
      {
        throw std::runtime_error(
          "DummyKeyProvider: key file has invalid length (" + std::to_string(bytes.size()) +
          "); expected " + std::to_string(need) + " bytes: " + p.string()
                                );
      }
    }
};


inline void generate_dummy_keys(const DatabaseKeyPolicy& policy,
                                const std::filesystem::path& root_dir,
                                bool overwrite = false)
{
  if (policy.empty())
    throw std::runtime_error("generate_dummy_keys: policy is empty");
  if (root_dir.empty())
    throw std::runtime_error("generate_dummy_keys: root_dir is empty");
  
  CryptoPP::AutoSeededRandomPool rng;
  
  const auto collections = policy.collectionNames();
  for (const auto& coll : collections)
  {
    const auto collPolicy = policy.collectionKeyPolicy(coll);
    
    auto emit = [&](EncryptionProfile profile, std::size_t nbytes)
    {
      const auto key_id = collPolicy.keyId(profile);
      
      std::filesystem::path rel{key_id};
      if (rel.is_absolute())
        throw std::runtime_error("generate_dummy_keys: keyId must be relative, got: " + rel.string());
      
      auto out_path = root_dir / rel;
      std::filesystem::create_directories(out_path.parent_path());
      
      if (!overwrite && std::filesystem::exists(out_path))
        return;
      
      std::vector<std::uint8_t> bytes(nbytes);
      if (nbytes != 0)
        rng.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(bytes.data()), bytes.size());
      
      std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
      if (!out)
        throw std::runtime_error("generate_dummy_keys: cannot write key file: " + out_path.string());
      
      if (!bytes.empty())
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
      
      if (!out)
        throw std::runtime_error("generate_dummy_keys: failed to write key file: " + out_path.string());
    };
    
    emit(EncryptionProfile::AES256_CTR, 32);
    emit(EncryptionProfile::AES192_CTR, 24);
    emit(EncryptionProfile::OPE_32_64,  32);
  }
}

#endif // MONGO_2_DUMMYKEYPROVIDER_H