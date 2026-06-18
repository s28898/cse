
#ifndef MONGO_2_STATEMENT_TRANSFORMER_V2_H
#define MONGO_2_STATEMENT_TRANSFORMER_V2_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/array/value.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>

#include "EncryptionService_v2.h"
#include "../PropertyMetadataStorage.h"
#include "../VariantJsonValue.h"
#include "../util.h"

struct StatementTransformerV2
{
    using ordered_json = nlohmann::ordered_json;

    static constexpr auto k_array_placeholder = "[0]";

    StatementTransformerV2(std::string collectionName,
                           std::shared_ptr<EncryptionServiceV2> encryptionService,
                           std::shared_ptr<PropertyMetadataStorage> metadata)
      : m_collectionName(std::move(collectionName))
      , m_encryption(std::move(encryptionService))
      , m_metadata(std::move(metadata))
    {
      if (m_collectionName.empty())
        throw std::runtime_error("StatementTransformerV2: empty collectionName");
      if (!m_encryption)
        throw std::runtime_error("StatementTransformerV2: encryptionService == null");
      if (!m_metadata)
        throw std::runtime_error("StatementTransformerV2: metadata == null");
    }


    static auto current_path_to_string(const std::vector<std::string>& current_path) -> std::string
    {
      std::stringstream ss;
      ss << '[';
      for (std::size_t i = 0; i < current_path.size(); ++i)
      {
        ss << current_path[i];
        if (i + 1 < current_path.size()) ss << ", ";
      }
      ss << ']';
      return ss.str();
    }


    inline void push_path(std::string node) { m_current_path.emplace_back(std::move(node)); }
    inline void pop_path()
    {
      if (m_current_path.empty())
        throw std::runtime_error("StatementTransformerV2: pop_path on empty stack");
      m_current_path.pop_back();
    }

    auto split_on_dot_strict(std::string_view s) -> std::vector<std::string>
    {
      std::vector<std::string> out;

      if (s.empty())
        throw std::runtime_error("split_on_dot_strict: empty path node, current=" + current_path_to_string(m_current_path));
      if (s.front() == '.' || s.back() == '.')
        throw std::runtime_error("split_on_dot_strict: leading/trailing dot in '" + std::string(s) +
                                 "', current=" + current_path_to_string(m_current_path));

      std::size_t start = 0;
      while (true)
      {
        const std::size_t pos = s.find('.', start);
        if (pos == std::string_view::npos)
        {
          out.emplace_back(s.substr(start));
          return out;
        }

        if (pos == start)
        {
          throw std::runtime_error("split_on_dot_strict: consecutive dots in '" + std::string(s) +
                                   "', current=" + current_path_to_string(m_current_path));
        }

        out.emplace_back(s.substr(start, pos - start));
        start = pos + 1;
      }
    }

    inline bool current_path_is_array_field() const
    {
      std::vector<std::string> probe = m_current_path;
      probe.emplace_back(k_array_placeholder);

      auto starts_with = [&](const std::vector<std::string>& full) -> bool {
        if (full.size() < probe.size()) return false;
        return std::equal(probe.begin(), probe.end(), full.begin());
      };

      return std::ranges::find_if(m_metadata->m_paths, starts_with) != m_metadata->m_paths.end();
    }

    template<class Fn>
    auto with_path(std::string path_node, Fn&& fn) -> void
    {
      auto segs = split_on_dot_strict(path_node);

      std::size_t pushed = 0;
      for (std::size_t i = 0; i < segs.size(); ++i)
      {
        m_current_path.emplace_back(std::move(segs[i]));
        ++pushed;

        const bool has_more = (i + 1) < segs.size();
        if (has_more && current_path_is_array_field())
        {
          m_current_path.emplace_back(k_array_placeholder);
          ++pushed;
        }
      }

      std::forward<Fn>(fn)();

      for (std::size_t i = 0; i < pushed; ++i)
        pop_path();
    }

    static inline bool is_keyword(std::string_view key)
    {
      return !key.empty() && key.front() == '$';
    }


    inline bool path_matches() const
    {
      auto eq = [&](const std::vector<std::string>& p) { return std::ranges::equal(p, m_current_path); };
      return std::ranges::find_if(m_metadata->m_paths, eq) != m_metadata->m_paths.end();
    }

    enum class EncKind
    {
        AES256_GCM,
        AES192_GCM,
        OPE_32_64
    };

    inline EncKind encryption_kind_for_current_path() const
    {
      auto eq = [&](const std::vector<std::string>& p) { return std::ranges::equal(p, m_current_path); };
      auto it = std::ranges::find_if(m_metadata->paths(), eq);
      if (it == m_metadata->paths().end())
        throw std::runtime_error("encryption_kind_for_current_path: path does not match, current=" +
                                 current_path_to_string(m_current_path));

      const auto idx = static_cast<std::size_t>(std::distance(m_metadata->paths().begin(), it));
      if (idx >= m_metadata->encryptions.size())
        throw std::runtime_error("encryption_kind_for_current_path: metadata.encryptions out of range");

      const std::string& s = m_metadata->encryptions.at(idx);

      if (s == "AES256_GCM" || s == "AES256")
        return EncKind::AES256_GCM;
      if (s == "AES192_GCM" || s == "AES192")
        return EncKind::AES192_GCM;

      if (s == "AES")
        return EncKind::AES256_GCM;

      if (s == "OPE_32_64" || s == "OPE")
        return EncKind::OPE_32_64;

      throw std::runtime_error("unsupported encryption kind '" + s + "' at path " + current_path_to_string(m_current_path));
    }


    static inline bsoncxx::types::b_binary make_binary_from_bytes(const std::string& bytes)
    {
      if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        throw std::runtime_error("make_binary_from_bytes: too large");

      bsoncxx::types::b_binary bin{};
      bin.sub_type = bsoncxx::binary_sub_type::k_binary;
      bin.size = static_cast<std::uint32_t>(bytes.size());
      bin.bytes = reinterpret_cast<const std::uint8_t*>(bytes.data());
      return bin;
    }

    static inline std::string aes_plaintext_bytes_from_json(const ordered_json& v)
    {
      if (v.is_string()) return v.get<std::string>();
      return v.dump();
    }

    inline bsoncxx::types::b_binary encrypt_aes_value(const ordered_json& v, EncKind kind)
    {
      const auto plaintext = aes_plaintext_bytes_from_json(v);

      EncryptionServiceV2::AesKind ak =
        (kind == EncKind::AES192_GCM) ? EncryptionServiceV2::AesKind::AES192_GCM
                                      : EncryptionServiceV2::AesKind::AES256_GCM;

      const std::string cipher = m_encryption->encrypt_aes(m_collectionName, ak, plaintext);
      return make_binary_from_bytes(cipher);
    }

    inline bsoncxx::types::b_int64 encrypt_ope_value(const ordered_json& v)
    {
      if (!v.is_number_unsigned() && !v.is_number_integer())
        throw std::runtime_error("OPE_32_64 expects integer at path " + current_path_to_string(m_current_path));

      std::int64_t signed_val = 0;
      if (v.is_number_unsigned())
      {
        const auto u = v.get<VariantJsonValue::number_unsigned_t>();
        if (u > static_cast<VariantJsonValue::number_unsigned_t>(std::numeric_limits<std::int32_t>::max()))
          throw std::runtime_error("OPE_32_64: value too large for int32 domain at path " + current_path_to_string(m_current_path));
        signed_val = static_cast<std::int64_t>(u);
      }
      else
      {
        signed_val = v.get<VariantJsonValue::number_integer_t>();
      }

      if (signed_val < 0)
        throw std::runtime_error("OPE_32_64 supports only non-negative int32 at path " + current_path_to_string(m_current_path));
      if (signed_val > std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("OPE_32_64: value too large for int32 domain at path " + current_path_to_string(m_current_path));

      const auto c = m_encryption->encrypt_ope_int32(m_collectionName, static_cast<std::int32_t>(signed_val));
      return bsoncxx::types::b_int64{static_cast<std::int64_t>(c)};
    }


    auto transform_array(const ordered_json& array, bool keyword_flag) -> bsoncxx::array::value
    {
      using bsoncxx::builder::basic::kvp;
      bsoncxx::builder::basic::array out{};

      for (const auto& value : array)
      {
        if (value.is_object())
        {
          const auto fn = [&] { out.append(transform_object(value)); };
          if (keyword_flag) fn();
          else with_path(k_array_placeholder, fn);
          continue;
        }

        if (value.is_array())
        {
          const auto fn = [&] { out.append(transform_array(value, keyword_flag)); };
          if (keyword_flag) fn();
          else with_path(k_array_placeholder, fn);
          continue;
        }

        const auto fn = [&]
        {
          if (path_matches())
          {
            const auto kind = encryption_kind_for_current_path();
            if (kind == EncKind::OPE_32_64)
              out.append(encrypt_ope_value(value));
            else
              out.append(encrypt_aes_value(value, kind));
          }
          else
          {
            auto variant = VariantJsonValue::make(value);
            std::visit(
              overloaded{
                [&out](VariantJsonValue::number_integer_t num) { out.append(num); },
                [&out](VariantJsonValue::number_float_t num) { out.append(num); },
                [&out](VariantJsonValue::number_unsigned_t num) { out.append(static_cast<std::int64_t>(num)); },
                [&out](VariantJsonValue::boolean_t b) { out.append(b); },
                [&out](std::string s) { out.append(std::move(s)); }
              },
              variant
                      );
          }
        };

        if (keyword_flag) fn();
        else with_path(k_array_placeholder, fn);
      }

      return out.extract();
    }

    auto transform_object(const ordered_json& object) -> bsoncxx::document::value
    {
      if (!object.is_object())
        throw std::runtime_error(std::string("transform_object: expected object, got ")
                                 + object.type_name()
                                 + ", current=" + current_path_to_string(m_current_path));

      using bsoncxx::builder::basic::kvp;
      bsoncxx::builder::basic::document out{};

      for (const auto& [key, value] : object.items())
      {
        if (value.is_object())
        {
          const auto fn = [&] { out.append(kvp(key, transform_object(value))); };

          if (key == "$elemMatch")
          {
            if (current_path_is_array_field()) with_path(k_array_placeholder, fn);
            else fn();
          }
          else
          {
            if (is_keyword(key)) fn();
            else with_path(key, fn);
          }
          continue;
        }

        if (value.is_array())
        {
          const bool kw = is_keyword(key);

          const auto fn = [&] { out.append(kvp(key, transform_array(value, kw))); };

          if ((key == "$in" || key == "$nin") && current_path_is_array_field())
          {
            with_path(k_array_placeholder, fn);
          }
          else
          {
            if (kw) fn();
            else with_path(key, fn);
          }
          continue;
        }

        const auto fn = [&]
        {
          if (path_matches())
          {
            const auto kind = encryption_kind_for_current_path();
            if (kind == EncKind::OPE_32_64)
            {
              out.append(kvp(key, encrypt_ope_value(value)));
            }
            else
            {
              out.append(kvp(key, encrypt_aes_value(value, kind)));
            }
            return;
          }

          if (value.is_string())
            out.append(kvp(key, value.get<std::string>()));
          else if (value.is_boolean())
            out.append(kvp(key, value.get<bool>()));
          else if (value.is_null())
            out.append(kvp(key, bsoncxx::types::b_null{}));
          else if (value.is_number_integer())
            out.append(kvp(key, value.get<std::int64_t>()));
          else if (value.is_number_unsigned())
            out.append(kvp(key, static_cast<std::int64_t>(value.get<VariantJsonValue::number_unsigned_t>())));
          else if (value.is_number_float())
            out.append(kvp(key, value.get<std::float_t>()));
          else
            throw std::runtime_error("unsupported value type at path " + current_path_to_string(m_current_path));
        };

        if (is_keyword(key)) fn();
        else with_path(key, fn);
      }

      return out.extract();
    }

    bsoncxx::document::value transform_statement_deep(const ordered_json& stmnt)
    {
      if (stmnt.is_null())
        throw std::runtime_error("transform_statement_deep: got null input statement");

      if (stmnt.is_object())
        return transform_object(stmnt);

      if (stmnt.is_array())
      {
        bsoncxx::builder::basic::document out{};
        out.append(bsoncxx::builder::basic::kvp("result", transform_array(stmnt, /*keyword_flag*/ true)));
        return out.extract();
      }

      throw std::runtime_error(
        std::string("transform_statement_deep: expected object/array, got ")
        + stmnt.type_name()
        + ", value=" + stmnt.dump()
        + ", current=" + current_path_to_string(m_current_path)
                              );
    }

    private:
    std::vector<std::string> m_current_path;

    std::string m_collectionName;
    std::shared_ptr<EncryptionServiceV2> m_encryption;
    std::shared_ptr<PropertyMetadataStorage> m_metadata;
};

#endif // MONGO_2_STATEMENT_TRANSFORMER_V2_H
