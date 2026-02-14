//
// Created by Bruno Kuś on 10/02/2026.
//


#ifndef MONGO_2_KEYPOLICY_H
#define MONGO_2_KEYPOLICY_H

#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "AbstractKeyProvider.h"
#include "EncryptionProfile.h"

// {
//   "databaseName": "...",
//   "policies": [
//     {
//       "collectionName": "...",
//       "keyId": {
//         "AES256_GCM": "...",
//         "AES192_GCM": "...",
//         "OPE_32_64": "..."
//       }
//     }
//   ]
// }

struct CollectionKeyPolicy
{
    CollectionKeyPolicy() = default;
    
    explicit CollectionKeyPolicy(nlohmann::json policy_obj)
      : m_policy(std::move(policy_obj))
    {
    }
    
    [[nodiscard]] std::string collectionName() const
    {
      ensure_initialized();
      return m_policy.at("collectionName").get<std::string>();
    }
    
    
    [[nodiscard]] std::string keyId(EncryptionProfile profile) const
    {
      ensure_initialized();
      const auto &keyIdObj = m_policy.at("keyId");
      
      const char *key = nullptr;
      switch (profile)
      {
        case EncryptionProfile::AES256_CTR:
          key = "AES256_CTR";
          break;
        case EncryptionProfile::AES192_CTR:
          key = "AES192_CTR";
          break;
        case EncryptionProfile::OPE_32_64:
          key = "OPE_32_64";
          break;
        default:
          throw std::runtime_error("CollectionKeyPolicy::keyId: unsupported profile");
      }
      
      return keyIdObj.at(key).get<std::string>();
    }
    
    [[nodiscard]] bool empty() const noexcept { return m_policy.is_null(); }
    
    private:
    nlohmann::json m_policy = nullptr;
    
    void ensure_initialized() const
    {
      if (m_policy.is_null())
      {
        throw std::runtime_error("CollectionKeyPolicy: used before initialization");
      }
    }
};

struct DatabaseKeyPolicy
{
    DatabaseKeyPolicy() = default;
    
    static DatabaseKeyPolicy fromFile(std::string_view path)
    {
      std::ifstream in(std::string{path});
      if (!in)
      {
        throw std::runtime_error(std::string("DatabaseKeyPolicy::fromFile: cannot open file: ") + std::string(path));
      }
      
      nlohmann::json root;
      try
      {
        in >> root;
      }
      catch (const std::exception &e)
      {
        throw std::runtime_error(std::string("DatabaseKeyPolicy::fromFile: JSON parse error: ") + e.what());
      }
      
      validate_root(root);
      
      std::vector<std::string> collectionNames;
      for (const auto &[key, value]: root.at("policies").items())
      {
        collectionNames.push_back(value.at("collectionName").get<std::string>());
      }
      DatabaseKeyPolicy out;
      
      out.m_collectionNames = std::move(collectionNames);
      
      out.m_root = std::move(root);
      
      
      return out;
    }
    
    [[nodiscard]] const std::vector<std::string> &collectionNames() const
    {
      ensure_initialized();
      return m_collectionNames;
    }
    
    [[nodiscard]] std::string databaseName() const
    {
      ensure_initialized();
      return m_root.at("databaseName").get<std::string>();
    }
    
    
    [[nodiscard]] CollectionKeyPolicy collectionKeyPolicy(std::string_view collection) const
    {
      ensure_initialized();
      
      const auto &policies = m_root.at("policies");
      for (const auto &p: policies)
      {
        if (p.at("collectionName").get<std::string>() == collection)
        {
          return CollectionKeyPolicy{p};
        } // copy sub-object as requested
      }
      
      throw std::runtime_error(std::string("DatabaseKeyPolicy: no policy for collection: ") + std::string(collection));
    }
    
    [[nodiscard]] bool empty() const noexcept { return m_root.is_null(); }
    
    private:
    std::vector<std::string> m_collectionNames;
    nlohmann::json m_root = nullptr;
    
    void ensure_initialized() const
    {
      if (m_root.is_null())
      {
        throw std::runtime_error("DatabaseKeyPolicy: used before initialization");
      }
    }
    
    static void ensure_exact_keys(
      const nlohmann::json &obj,
      std::initializer_list<const char *> allowed,
      std::string_view ctx
    )
    {
      if (!obj.is_object())
      {
        throw std::runtime_error(std::string(ctx) + ": expected object");
      }
      
      std::unordered_set<std::string> allowed_set;
      allowed_set.reserve(allowed.size());
      for (auto *k: allowed) allowed_set.emplace(k);
      
      for (const auto &[k, _]: obj.items())
      {
        if (!allowed_set.contains(k))
        {
          throw std::runtime_error(std::string(ctx) + ": unexpected key: " + k);
        }
      }
      
      for (auto *k: allowed)
      {
        if (!obj.contains(k))
        {
          throw std::runtime_error(std::string(ctx) + ": missing key: " + std::string(k));
        }
      }
    }
    
    static void validate_key_id(const nlohmann::json &keyId, std::string_view ctx)
    {
      ensure_exact_keys(keyId, {"AES256_CTR", "AES192_CTR", "OPE_32_64"}, ctx);
      
      for (auto *k: {"AES256_CTR", "AES192_CTR", "OPE_32_64"})
      {
        if (!keyId.at(k).is_string())
        {
          throw std::runtime_error(std::string(ctx) + ": keyId." + k + " must be string");
        }
        
        const auto s = keyId.at(k).get<std::string>();
        if (s.empty())
        {
          throw std::runtime_error(std::string(ctx) + ": keyId." + k + " must be non-empty");
        }
      }
    }
    
    static void validate_policy(const nlohmann::json &policy, std::string_view ctx)
    {
      ensure_exact_keys(policy, {"collectionName", "keyId"}, ctx);
      
      if (!policy.at("collectionName").is_string())
      {
        throw std::runtime_error(std::string(ctx) + ": collectionName must be string");
      }
      
      const auto coll = policy.at("collectionName").get<std::string>();
      if (coll.empty())
      {
        throw std::runtime_error(std::string(ctx) + ": collectionName must be non-empty");
      }
      
      validate_key_id(policy.at("keyId"), std::string(ctx) + ".keyId");
    }
    
    static void validate_root(const nlohmann::json &root)
    {
      ensure_exact_keys(root, {"databaseName", "policies"}, "DatabaseKeyPolicy root");
      
      if (!root.at("databaseName").is_string())
      {
        throw std::runtime_error("DatabaseKeyPolicy root: databaseName must be string");
      }
      
      const auto db = root.at("databaseName").get<std::string>();
      if (db.empty())
      {
        throw std::runtime_error("DatabaseKeyPolicy root: databaseName must be non-empty");
      }
      
      if (!root.at("policies").is_array())
      {
        throw std::runtime_error("DatabaseKeyPolicy root: policies must be array");
      }
      
      const auto &policies = root.at("policies");
      if (policies.empty())
      {
        throw std::runtime_error("DatabaseKeyPolicy root: policies must contain at least one element");
      }
      
      std::unordered_set<std::string> seen;
      for (std::size_t i = 0; i < policies.size(); ++i)
      {
        const auto &p = policies.at(i);
        const std::string ctx = "DatabaseKeyPolicy root.policies[" + std::to_string(i) + "]";
        validate_policy(p, ctx);
        
        const auto coll = p.at("collectionName").get<std::string>();
        if (!seen.emplace(coll).second)
        {
          throw std::runtime_error("DatabaseKeyPolicy root: duplicate collectionName: " + coll);
        }
      }
    }
};

#endif //MONGO_2_KEYPOLICY_H