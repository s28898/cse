//
// Created by Bruno Kuś on 05/11/2025.
//

#ifndef MONGO_1_STATEMENT_TRANSFORMER_H
#define MONGO_1_STATEMENT_TRANSFORMER_H

// BSON includes
#include <bsoncxx/json.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>


#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include <nlohmann/json.hpp>
#include "EncryptionService_v1.h"
//#include "EncryptionService_v2.h"
#include "VariantJsonValue.h"

#include "util.h"
#include "PropertyMetadataStorage.h"
    #include "./TodoError.h"

using namespace nlohmann;

bsoncxx::types::b_binary binary_from_string(std::string ciphertext);

#ifndef __cpp_lib_ranges_join_with
//static_assert(false);
#endif

struct StatementTransformer
{
    
    StatementTransformer(EncryptionService encryption_agent_1, PropertyMetadataStorage meta)
      : encryption_agent{std::move(encryption_agent_1)}, metadata(std::move(meta))
    {
//      encryption_agent.load("../secret/context.bin");
    }
    
    static auto current_path_to_string(std::vector<std::string> current_path) -> std::string
    {
      std::stringstream ss;
      ss << '[';
      if (!current_path.empty())
      {
        ss << *current_path.begin();
        if (current_path.size() > 1)
        {
          for (auto second = current_path.begin() + 1; second < current_path.end(); ++second)
          {
            ss << ", " << *second;
          }
        }
      }
      ss << ']';
      return ss.str();
    }

//    std::vector<std::vector<std::string>> m_paths;
    std::vector<std::string> m_current_path;
    
    inline void push_path(std::string path_node) { m_current_path.emplace_back(std::move(path_node)); }
    
    inline void pop_path() { m_current_path.pop_back(); }
    
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
        std::size_t pos = s.find('.', start);
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
      
      return std::ranges::find_if(metadata.m_paths, starts_with) != metadata.m_paths.end();
    }
//    auto with_path(std::string path_node, auto fn) -> void
//    {
//      std::vector<std::string> node_segments = split_on_dot_strict(path_node);
//      const std::size_t pushed_total = node_segments.size();
//      for (auto &node_segment: node_segments) { m_current_path.emplace_back(std::move(node_segment)); }
//      fn();
//      for (std::size_t i = 0; i < pushed_total; ++i) {
//        pop_path();
//      }
//    }
    auto with_path(std::string path_node, auto fn) -> void
    {
      auto node_segments = split_on_dot_strict(path_node);
      
      std::size_t pushed_total = 0;
      
      for (std::size_t i = 0; i < node_segments.size(); ++i)
      {
        m_current_path.emplace_back(std::move(node_segments[i]));
        ++pushed_total;
        
        const bool has_more = (i + 1) < node_segments.size();
        if (has_more && current_path_is_array_field())
        {
          m_current_path.emplace_back(k_array_placeholder);
          ++pushed_total;
        }
      }
      
      fn();
      
      for (std::size_t i = 0; i < pushed_total; ++i)
        pop_path();
    }
    
    
    EncryptionService encryption_agent;
    
    PropertyMetadataStorage metadata;
    
 
    
    inline
    bool path_matches()
    {
      std::cout << "===StatementTransformer::path_matches() ===\n";
      auto do_paths_match = [&](const std::vector<std::string> &path)
      {
        return std::ranges::equal(path, m_current_path);
      };
      auto path_matches = std::ranges::find_if(metadata.m_paths, do_paths_match) != metadata.m_paths.end();
      std::cout << "m_current_path == ";
      for (const auto &path_node: m_current_path)
      {
        std::cout << path_node << " ";
      }
      std::cout << "\n";
      std::cout << std::boolalpha << "path_matches == " << path_matches << "\n";
//      std::cout << current_path_to_string(m_current_path) << '\n';
      return path_matches;
    }
    
    
    enum class EncryptionKind
    {
        AES, OPE
    };
    
    auto get_encryption_if_path_matches() -> EncryptionKind
    {
      for (const auto &encs: metadata.encryptions)
      {
        std::cout << "encs: " << encs << std::endl;
      }
      for (const auto &typs: metadata.types)
      {
        std::cout << "typs: " << typs << std::endl;
      }
      
      
      if (not path_matches()) throw TodoError();
      auto do_paths_match =
        [&](const std::vector<std::string> &path) { return std::ranges::equal(path, m_current_path); };
      auto path_matches = std::ranges::find_if(metadata.paths(), do_paths_match);
      auto pos = std::distance(metadata.paths().begin(), path_matches);
      std::cerr << "enc pos : " << pos << '\n';
      std::cerr << "enc size : " << metadata.encryptions.size() << '\n';
      
      auto encryption_as_str = metadata.encryptions.at(pos);
      if (encryption_as_str == "AES") { return EncryptionKind::AES; }
      else if (encryption_as_str == "OPE") { return EncryptionKind::OPE; }
      else { throw TodoError(); }
      
    }
    
    
    auto binary_from_json_value_aes(const ordered_json &ordered_json) -> bsoncxx::types::b_binary
    {
      
      if (ordered_json.is_string())
      {
        return binary_from_string(encryption_agent.encrypt_AES(ordered_json));
      }
      else if (ordered_json.is_number_float())
      {
        return binary_from_string(
          encryption_agent.encrypt_AES(ordered_json.get<float_t>()));
      }
      else if (ordered_json.is_number_integer())
      {
        std::cout << "encrypting number integer\n";
        auto encrypted = encryption_agent.encrypt_AES(ordered_json.get<int64_t>());
        auto decrypted = encryption_agent.decrypt_AES(encrypted);

//        std::cout << "value : " << ordered_json.get<int64_t>() << '\n';
//        std::cout << "encrypted : " << encrypted << '\n';
//        std::cout << "decrypted : " << decrypted << '\n';
        return binary_from_string(
//          encryption_agent.encrypt_AES(ordered_json)
          encryption_agent.encrypt_AES(ordered_json.get<int64_t>())
                                 );
      }
      else { throw std::runtime_error(""); }
    };
    static constexpr auto k_array_placeholder = "[0]";
    
    auto transform_array(const ordered_json &array, bool keyword_flag) -> bsoncxx::array::value
    {
      using namespace bsoncxx::builder::basic;
      using bsoncxx::builder::basic::kvp;
      bsoncxx::builder::basic::array result{};
      for (const auto &[index, value]: array.items())
      {
        if (value.is_object())
        {
          if (value.empty())
          {
            result.append(make_document());
            continue;
          }
//          push_path(k_array_placeholder);
          const auto fn = [&] { result.append(transform_object(value)); };
          if (keyword_flag) { fn(); }
          else { with_path(k_array_placeholder, fn); }
//          pop_path();
        }
        else if (value.is_array())
        {
//          push_path(k_array_placeholder);
          const auto fn = [&] { result.append(transform_array(value, keyword_flag)); };
          if (keyword_flag) { fn(); }
          else { with_path(k_array_placeholder, fn); }
//          pop_path();
        }
        else
        {

//          push_path(k_array_placeholder);
          const auto fn = [&]
          {
            if (not(value.is_number() or value.is_string())) throw std::runtime_error("unsupported value");
            if ((value == "secretA" || value == "secretB") && not path_matches())
            {
//             throw std::runtime_error("path should match: " + current_path_to_string(m_current_path));
            }
            if (path_matches())
            {
              auto encryption_kind = get_encryption_if_path_matches();
              
              if (encryption_kind == EncryptionKind::OPE)
              {
                if (not value.is_number_unsigned()) throw std::runtime_error("");
                
                auto int_value = static_cast<int>(value.get<VariantJsonValue::number_unsigned_t>());
                
                auto cipher_int64 = encryption_agent.encrypt_int32(int_value);
                auto cipher_uint64 = static_cast<int64_t>(cipher_int64);
                auto cipher_bson = bsoncxx::types::b_int64{cipher_uint64};
                result.append(cipher_bson);
              }
              else
              {
                result.append(binary_from_json_value_aes(value));
              }
            }
            else
            {
              auto variant = VariantJsonValue::make(value);
              
              std::visit(
                overloaded{
                  [&result](VariantJsonValue::number_integer_t num) { result.append(num); },
                  [&result](VariantJsonValue::number_float_t num) { result.append(num); },
                  [&result](VariantJsonValue::number_unsigned_t num)
                  {
                    result.append(
                      static_cast<std::int64_t >(num));
                  },
                  [&result](VariantJsonValue::boolean_t boolean) { result.append(boolean); },
                  [&result](std::string value)
                  {
                    auto leak = new std::string(std::move(value));
                    result.append(*leak);
                  }
                },
                variant
                        );
            }
          };
          if (keyword_flag) { fn(); }
          else { with_path(k_array_placeholder, fn); }
//          pop_path();
        }
      }
      return result.extract();
    };
    
    static auto is_keyword(std::string_view key) -> bool
    {
      std::cout << "[is_keyword] " << "is_keyword(" << key << ") -> " << std::boolalpha << key.starts_with('$') << '\n';
      return key.starts_with('$');
    }
    
    auto transform_object(const ordered_json &object) -> bsoncxx::document::value
    { // NOLINT
      if (!object.is_object())
        throw std::runtime_error(std::string("transform_object: expected object, got ") + object.type_name() +
                                 ", current=" + current_path_to_string(m_current_path));
      using namespace bsoncxx::builder::basic;
      document result{};
      for (const auto &[key, value]: object.items())
      {
        
        if (value.is_object())
        {
          std::cout << "===value.is_object()===\n";
          if (value.empty())
          {
            result.append(kvp(key, make_document()));
            continue;
          }
          
          const auto fn = [&] { result.append(kvp(key, transform_object(value))); };
          
          // SPECIAL CASE:
          // $elemMatch is a field-level operator that conceptually enters "some array element".
          // Our schema encodes that as an explicit "[0]" path node (e.g., items.[0].secretNote),
          // so we must inject the placeholder when descending into $elemMatch's predicate object.
          if (key == "$elemMatch")
          {
            if (current_path_is_array_field()) {
              with_path(k_array_placeholder, fn);   // <-- inject "[0]" here
            } else {
              fn();
            }
          }
          else
          {
            if (is_keyword(key)) { fn(); }
            else { with_path(key, fn); }
          }
        }
//        if (value.is_object())
//        {
//          std::cout << "===value.is_object()===\n";
//          if (value.empty())
//          {
//            result.append(kvp(key, make_document()));
//            continue;
//          }
//
////          push_path(was_object ? key : k_array_placeholder);
//          const auto fn = [&] { result.append(kvp(key, transform_object(value))); };
//          if (is_keyword(key)) { fn(); }
//          else { with_path(was_object ? key : k_array_placeholder, fn); }
//
////          pop_path();
//        }
        else if (value.is_array())
        {
          if (value.empty()) {
            result.append(kvp(key, make_array()));
            continue;
          }
          
          const bool kw = is_keyword(key);
          
          const auto fn = [&] {
            result.append(kvp(key, transform_array(value, kw)));
          };
          
          // SPECIAL CASE:
          // Under an array field (e.g. "tags"), $in/$nin compares against array ELEMENTS.
          // Our schema encodes that as ["tags","[0]"], so inject the placeholder.
          if ((key == "$in" || key == "$nin") && current_path_is_array_field()) {
            with_path(k_array_placeholder, fn);  // push "[0]" while processing candidates
          } else {
            if (kw) fn();
            else with_path(key, fn);
          }
        }
//        else if (value.is_array())
//        {
//          if (value.empty())
//          {
//            result.append(kvp(key, make_array()));
//            continue;
//          }
////          push_path(was_object ? key : k_array_placeholder);
////          result.append(kvp(key, transform_object(value)));
//          const auto fn = [&] { result.append(kvp(key, transform_array(value, is_keyword(key)))); };
//          if (is_keyword(key)) { fn(); }
//          else { with_path(was_object ? key : k_array_placeholder, fn); }
////          pop_path();
//        }
        else
        {
//          push_path(was_object ? key : k_array_placeholder);
          const auto fn = [&] -> void
          {
            if (path_matches())
            {
              auto encryption_kind = get_encryption_if_path_matches();
              
              if (encryption_kind == EncryptionKind::OPE)
              {
                if (not value.is_number_unsigned())
                  throw std::runtime_error("OPE expects unsigned integer at path " + current_path_to_string(m_current_path));
                
                auto int_value = static_cast<int>(value.get<VariantJsonValue::number_unsigned_t>());
                
                auto cipher_int64 = encryption_agent.encrypt_int32(int_value);
                auto cipher_uint64 = static_cast<int64_t>(cipher_int64);
                auto cipher_bson = bsoncxx::types::b_int64{cipher_uint64};
                result.append(kvp(key, cipher_bson));
              }
              else
              {
                result.append(kvp(key, binary_from_json_value_aes(value)));
              }
            }
            else
            {
              if (value.is_string())
              {
                result.append(kvp(key, value.get<std::string>()));
              }
              else if (value.is_boolean())
              {
                result.append(kvp(key, value.get<bool>()));
              }
              else if (value.is_null())
              {
                result.append(kvp(key, bsoncxx::types::b_null{}));
              }
              else if (value.is_number_integer())
              {
                result.append(kvp(key, value.get<std::int64_t>()));
              }
              else if (value.is_number_unsigned())
              {
                // prefer int64 for large unsigned values
                result.append(kvp(key, static_cast<std::int64_t>(value.get<VariantJsonValue::number_unsigned_t>())));
              }
              else if (value.is_number_float())
              {
                result.append(kvp(key, value.get<std::float_t>()));
              }
              else
              {
                throw std::runtime_error("unsupported value type at path " + current_path_to_string(m_current_path));
              }
            }
          };
          if (is_keyword(key)) { fn(); }
          else { with_path(key, fn); }
//          pop_path();
        }
        
      }
      return result.extract();
      
    }
    
    
    bsoncxx::document::value
    transform_statement_deep(const ordered_json &stmnt)
    {
      if (stmnt.is_null())
        throw std::runtime_error("transform_statement_deep: got null input statement");
      if (stmnt.is_object())
      {
        auto result = transform_object(stmnt);
        static_assert(std::is_move_constructible_v<decltype(result)>);
        return result;
      }
      if (stmnt.is_array())
      {
        // Allow top-level arrays for internal use cases; treat them as a keywordless array context.
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
};


#endif //MONGO_1_STATEMENT_TRANSFORMER_H
