
#ifndef MONGO_2_BSONDECRYPTORV3_H
#define MONGO_2_BSONDECRYPTORV3_H

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <bsoncxx/array/view.hpp>
#include <bsoncxx/array/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types/bson_value/value.hpp>

#include "EncryptionServiceV3.h"
#include "../PropertyMetadataStorage.h"

struct BsonDecryptorV3
{
    static constexpr auto k_array_placeholder = "[0]";

    BsonDecryptorV3(std::string collectionName,
                    std::shared_ptr<EncryptionServiceV3> encryptionService,
                    std::shared_ptr<PropertyMetadataStorage> metadata)
      : m_collectionName(std::move(collectionName))
      , m_encryption(std::move(encryptionService))
      , m_metadata(std::move(metadata))
    {
      if (m_collectionName.empty())
        throw std::runtime_error("BsonDecryptorV3: empty collectionName");
      if (!m_encryption)
        throw std::runtime_error("BsonDecryptorV3: encryptionService == null");
      if (!m_metadata)
        throw std::runtime_error("BsonDecryptorV3: metadata == null");
    }

    bsoncxx::document::value decrypt_deep(bsoncxx::document::view document)
    {
      m_current_path.clear();
      return decrypt_object(document);
    }

    private:
    std::string m_collectionName;
    std::shared_ptr<EncryptionServiceV3> m_encryption;
    std::shared_ptr<PropertyMetadataStorage> m_metadata;

    std::vector<std::string> m_current_path;

    static std::string path_to_string(const std::vector<std::string>& p)
    {
      std::ostringstream ss;
      ss << '[';
      for (std::size_t i = 0; i < p.size(); ++i)
      {
        ss << p[i];
        if (i + 1 < p.size()) ss << ", ";
      }
      ss << ']';
      return ss.str();
    }

    inline void push_path(std::string node) { m_current_path.emplace_back(std::move(node)); }
    inline void pop_path()
    {
      if (m_current_path.empty())
        throw std::runtime_error("BsonDecryptorV3: pop_path on empty stack");
      m_current_path.pop_back();
    }

    inline bool path_matches() const
    {
      auto eq = [&](const std::vector<std::string>& p) { return std::ranges::equal(p, m_current_path); };
      return std::ranges::find_if(m_metadata->paths(), eq) != m_metadata->paths().end();
    }

    std::size_t matched_index() const
    {
      auto eq = [&](const std::vector<std::string>& p) { return std::ranges::equal(p, m_current_path); };
      auto it = std::ranges::find_if(m_metadata->paths(), eq);
      if (it == m_metadata->paths().end())
        throw std::runtime_error("BsonDecryptorV3: metadata missing path " + path_to_string(m_current_path));
      return static_cast<std::size_t>(std::distance(m_metadata->paths().begin(), it));
    }

    enum class EncKind
    {
        AES256_CTR,
        AES192_CTR,
        OPE_32_64
    };

    EncKind encryption_kind_for_path(std::size_t idx) const
    {
      if (idx >= m_metadata->encryptions.size())
        throw std::runtime_error("BsonDecryptorV3: encryptions index out of range");

      const std::string& s = m_metadata->encryptions.at(idx);

      if (s == "AES256_CTR" || s == "AES256")
        return EncKind::AES256_CTR;

      if (s == "AES192_CTR" || s == "AES192")
        return EncKind::AES192_CTR;

      if (s == "AES")
        return EncKind::AES256_CTR;

      if (s == "OPE_32_64" || s == "OPE")
        return EncKind::OPE_32_64;

      if (s.find("GCM") != std::string::npos)
        throw std::runtime_error("BsonDecryptorV3: AES-GCM not supported (got '" + s + "')");

      throw std::runtime_error("BsonDecryptorV3: unsupported encryption kind '" + s +
                               "' at path " + path_to_string(m_current_path));
    }

    enum class PlainType
    {
        String,
        Int32,
        Int64,
        Double,
        Unknown
    };

    PlainType plain_type_for_path(std::size_t idx) const
    {
      if (idx >= m_metadata->types.size())
        throw std::runtime_error("BsonDecryptorV3: types index out of range");

      const std::string& t = m_metadata->types.at(idx);
      if (t == "string") return PlainType::String;
      if (t == "int")    return PlainType::Int32;
      if (t == "long")   return PlainType::Int64;
      if (t == "double") return PlainType::Double;
      return PlainType::Unknown;
    }

    static EncryptionServiceV3::AesKind to_aes_kind(EncKind k)
    {
      return (k == EncKind::AES192_CTR)
             ? EncryptionServiceV3::AesKind::AES192_CTR
             : EncryptionServiceV3::AesKind::AES256_CTR;
    }

    static std::int64_t parse_int64_strict(std::string_view s)
    {
      std::size_t pos = 0;
      long long v = 0;
      try
      {
        v = std::stoll(std::string(s), &pos, 10);
      }
      catch (...)
      {
        throw std::runtime_error("BsonDecryptorV3: failed to parse int64 plaintext '" + std::string(s) + "'");
      }
      if (pos != s.size())
        throw std::runtime_error("BsonDecryptorV3: trailing chars in int plaintext '" + std::string(s) + "'");
      return static_cast<std::int64_t>(v);
    }

    static double parse_double_strict(std::string_view s)
    {
      std::size_t pos = 0;
      double v = 0.0;
      try
      {
        v = std::stod(std::string(s), &pos);
      }
      catch (...)
      {
        throw std::runtime_error("BsonDecryptorV3: failed to parse double plaintext '" + std::string(s) + "'");
      }
      if (pos != s.size())
        throw std::runtime_error("BsonDecryptorV3: trailing chars in double plaintext '" + std::string(s) + "'");
      return v;
    }

//    std::string decrypt_aes_to_string(const bsoncxx::types::b_binary& bin, EncKind enc)
//    {
//      // EncryptionServiceV3 already parses IV from blob.
//      return m_encryption->decrypt_aes(m_collectionName, to_aes_kind(enc), bin);
//    }
//    std::string decrypt_aes_to_string(const bsoncxx::types::b_binary& bin, EncKind enc)
//    {
//      // build property_path like in StatementTransformerV3
//      std::string property_path;
//      property_path.reserve(m_current_path.size() * 7);
//      for (std::size_t i = 0; i < m_current_path.size(); ++i)
//      {
//        if (i) property_path.push_back('.');
//        property_path.append(m_current_path[i]);
//      }
//
//      return m_encryption->decrypt_aes(m_collectionName, property_path, to_aes_kind(enc), bin);
//    }
    std::string decrypt_aes_to_string(const bsoncxx::types::b_binary& bin, EncKind enc)
    {
      std::string property_path;
      property_path.reserve(m_current_path.size() * 7);
      for (std::size_t i = 0; i < m_current_path.size(); ++i)
      {
        if (i) property_path.push_back('.');
        property_path.append(m_current_path[i]);
      }

      const auto total = static_cast<std::size_t>(bin.size);
      std::cerr << "[DEC] path=" << property_path
                << " total=" << total
                << " iv=";

      const auto* bytes = reinterpret_cast<const std::uint8_t*>(bin.bytes);
      for (std::size_t i = 0; i < std::min<std::size_t>(16, total); ++i)
        std::cerr << std::hex << (int)bytes[i] << ' ';
      std::cerr << std::dec << "\n";

      auto pt = m_encryption->decrypt_aes(m_collectionName, property_path, to_aes_kind(enc), bin);

      std::cerr << "[DEC] path=" << property_path
                << " pt.size=" << pt.size()
                << " pt.hex=" << hex_prefix(pt, 32) << "\n";

      return pt;
    }

    static bool is_valid_utf8(std::string_view s)
    {
      const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
      const unsigned char* end = p + s.size();

      while (p < end)
      {
        unsigned char c = *p++;
        if (c <= 0x7F) continue;

        int need = 0;
        if ((c >> 5) == 0x6) need = 1;
        else if ((c >> 4) == 0xE) need = 2;
        else if ((c >> 3) == 0x1E) need = 3;
        else return false;

        if (p + need > end) return false;

        for (int i = 0; i < need; ++i)
          if ((p[i] >> 6) != 0x2) return false;

        p += need;
      }
      return true;
    }

    static std::string hex_prefix(std::string_view s, std::size_t max_bytes = 64)
    {
      static const char* hex = "0123456789ABCDEF";
      std::string out;
      const std::size_t n = std::min(max_bytes, s.size());
      out.reserve(n * 3);

      for (std::size_t i = 0; i < n; ++i)
      {
        unsigned char b = static_cast<unsigned char>(s[i]);
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0xF]);
        if (i + 1 < n) out.push_back(' ');
      }

      if (s.size() > n) out.append(" ...");
      return out;
    }
    std::string decrypt_string_from_aes(const bsoncxx::types::b_binary& bin, EncKind enc)
    {
      const std::string plaintext = decrypt_aes_to_string(bin, enc);

      if (plaintext.find('\0') != std::string::npos)
      {
        throw std::runtime_error(
          "BsonDecryptorV3: decrypted string contains NUL byte at " + path_to_string(m_current_path) +
          " hex=" + hex_prefix(plaintext)
                                );
      }

      if (!is_valid_utf8(plaintext))
      {
        throw std::runtime_error(
          "BsonDecryptorV3: decrypted string is not valid UTF-8 at " + path_to_string(m_current_path) +
          " hex=" + hex_prefix(plaintext)
                                );
      }

      return plaintext;
    }

    bsoncxx::types::b_int32 decrypt_int32_from_aes(const bsoncxx::types::b_binary& bin, EncKind enc)
    {
      const auto plaintext = decrypt_aes_to_string(bin, enc);
      const auto v = parse_int64_strict(plaintext);
      if (v < std::numeric_limits<std::int32_t>::min() || v > std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("BsonDecryptorV3: AES int plaintext out of int32 range at " + path_to_string(m_current_path));
      return bsoncxx::types::b_int32{static_cast<std::int32_t>(v)};
    }

    bsoncxx::types::b_int64 decrypt_int64_from_aes(const bsoncxx::types::b_binary& bin, EncKind enc)
    {
      const auto plaintext = decrypt_aes_to_string(bin, enc);
      const auto v = parse_int64_strict(plaintext);
      return bsoncxx::types::b_int64{v};
    }

    bsoncxx::types::b_double decrypt_double_from_aes(const bsoncxx::types::b_binary& bin, EncKind enc)
    {
      const auto plaintext = decrypt_aes_to_string(bin, enc);
      const auto v = parse_double_strict(plaintext);
      return bsoncxx::types::b_double{v};
    }

    bsoncxx::types::b_int32 decrypt_int32_from_ope(const bsoncxx::types::b_int64& element)
    {
      if (element.value < 0)
        throw std::runtime_error("BsonDecryptorV3: OPE ciphertext is negative at " + path_to_string(m_current_path));

      const auto plain = m_encryption->decrypt_ope_uint64(m_collectionName,
                                                          static_cast<std::uint64_t>(element.value));
      return bsoncxx::types::b_int32{plain};
    }

    bsoncxx::document::value decrypt_object(bsoncxx::document::view object)
    {
      using bsoncxx::builder::basic::kvp;
      bsoncxx::builder::basic::document out{};

      for (const auto& el : object)
      {
        const std::string key = el.key().data();

        if (el.type() == bsoncxx::type::k_document)
        {
          auto sub = el.get_document().view();
          push_path(key);
          auto decrypted = decrypt_object(sub);
          out.append(kvp(key, decrypted.view()));
          pop_path();
          continue;
        }

        if (el.type() == bsoncxx::type::k_array)
        {
          auto arr = el.get_array().value;
          push_path(key);
          auto decrypted = decrypt_array(arr);
          out.append(kvp(key, decrypted.view()));
          pop_path();
          continue;
        }

        // primitive
        push_path(key);

        if (path_matches())
        {
          const auto idx = matched_index();
          const auto enc = encryption_kind_for_path(idx);
          const auto t = plain_type_for_path(idx);

          if (enc == EncKind::OPE_32_64)
          {
            if (el.type() != bsoncxx::type::k_int64)
              throw std::runtime_error("BsonDecryptorV3: expected int64 OPE ciphertext at " + path_to_string(m_current_path));
            out.append(kvp(key, decrypt_int32_from_ope(el.get_int64())));
            pop_path();
            continue;
          }

          // AES CTR
          if (el.type() != bsoncxx::type::k_binary)
            throw std::runtime_error("BsonDecryptorV3: expected binary AES ciphertext at " + path_to_string(m_current_path));
          const auto bin = el.get_binary();

          switch (t)
          {
            case PlainType::String:
              out.append(kvp(key, decrypt_string_from_aes(bin, enc)));
              break;
            case PlainType::Int32:
              out.append(kvp(key, decrypt_int32_from_aes(bin, enc)));
              break;
            case PlainType::Int64:
              out.append(kvp(key, decrypt_int64_from_aes(bin, enc)));
              break;
            case PlainType::Double:
              out.append(kvp(key, decrypt_double_from_aes(bin, enc)));
              break;
            case PlainType::Unknown:
            default:
              out.append(kvp(key, decrypt_string_from_aes(bin, enc)));
              break;
          }

          pop_path();
          continue;
        }

        out.append(kvp(el.key(), el.get_value()));
        pop_path();
      }

      return out.extract();
    }

    bsoncxx::array::value decrypt_array(bsoncxx::array::view array)
    {
      bsoncxx::builder::basic::array out{};

      for (const auto& el : array)
      {
        if (el.type() == bsoncxx::type::k_document)
        {
          push_path(k_array_placeholder);
          auto decrypted = decrypt_object(el.get_document().view());
          out.append(decrypted.view());
          pop_path();
          continue;
        }

        if (el.type() == bsoncxx::type::k_array)
        {
          push_path(k_array_placeholder);
          auto decrypted = decrypt_array(el.get_array().value);
          out.append(decrypted.view());
          pop_path();
          continue;
        }

        // primitive element
        push_path(k_array_placeholder);

        if (path_matches())
        {
          const auto idx = matched_index();
          const auto enc = encryption_kind_for_path(idx);
          const auto t = plain_type_for_path(idx);

          if (enc == EncKind::OPE_32_64)
          {
            if (el.type() != bsoncxx::type::k_int64)
              throw std::runtime_error("BsonDecryptorV3: expected int64 OPE ciphertext at " + path_to_string(m_current_path));
            out.append(decrypt_int32_from_ope(el.get_int64()));
            pop_path();
            continue;
          }

          if (el.type() != bsoncxx::type::k_binary)
            throw std::runtime_error("BsonDecryptorV3: expected binary AES ciphertext at " + path_to_string(m_current_path));
          const auto bin = el.get_binary();

          switch (t)
          {
            case PlainType::String:
              out.append(decrypt_string_from_aes(bin, enc));
              break;
            case PlainType::Int32:
              out.append(decrypt_int32_from_aes(bin, enc));
              break;
            case PlainType::Int64:
              out.append(decrypt_int64_from_aes(bin, enc));
              break;
            case PlainType::Double:
              out.append(decrypt_double_from_aes(bin, enc));
              break;
            case PlainType::Unknown:
            default:
              out.append(decrypt_string_from_aes(bin, enc));
              break;
          }

          pop_path();
          continue;
        }

        out.append(el.get_value());
        pop_path();
      }

      return out.extract();
    }
};

#endif // MONGO_2_BSONDECRYPTORV3_H
