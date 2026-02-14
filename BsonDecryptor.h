//
// Created by Bruno Kuś on 08/01/2026.
//

#ifndef MONGO_2_BSONDECRYPTOR_H
#define MONGO_2_BSONDECRYPTOR_H

#include <bsoncxx/document/value.hpp>
#include "EncryptionService_v1.h"
#include "PropertyMetadataStorage.h"
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/array/view.hpp>
#include <bsoncxx/types/bson_value/value.hpp>
#include "util.h"
#include "TodoError.h"

#include <source_location>

struct BsonDecryptor
{
    
    explicit BsonDecryptor
      (
        EncryptionService agent, PropertyMetadataStorage meta
      )
      : encryption_agent(std::move(agent)), metadata(std::move(meta)) {}
      
      
    std::vector<std::string> m_current_path;
    
    
    inline bool path_matches()
    {
      std::cout << "===StatementTransformer::path_matches() ===\n";
      auto do_paths_match = [&](const std::vector<std::string> &path)
      {
        return std::ranges::equal(path, m_current_path);
      };
      auto path_matches = std::ranges::find_if(metadata.paths(), do_paths_match) != metadata.paths().end();
      std::cout << "m_current_path == ";
      for (const auto &path_node: m_current_path)
      {
        std::cout << path_node << " ";
      }
      std::cout << "\n";
      std::cout << std::boolalpha << "path_matches == " << path_matches << "\n";
      return path_matches;
    }
    
    enum class EncryptionKind
    {
        AES, OPE
    };
    
    auto get_encryption_if_path_matches() -> EncryptionKind
    {
      if (not path_matches()) throw TodoError();
//      if (not path_matches()) throw std::runtime_error("[BsonDecryptor]");
      auto do_paths_match =
        [&](const std::vector<std::string> &path) { return std::ranges::equal(path, m_current_path); };
      auto path_matches = std::ranges::find_if(metadata.paths(), do_paths_match);
      auto pos = std::distance(metadata.paths().begin(), path_matches);
      auto encryption_as_str = metadata.encryptions.at(pos);
      if (encryption_as_str == "AES") { return EncryptionKind::AES; }
      else if (encryption_as_str == "OPE") { return EncryptionKind::OPE; }
      else { throw TodoError(); }
//      else { throw std::runtime_error("[BsonDecryptor]"); }
    
    }
    
    auto get_type_if_path_matches() -> bsoncxx::v_noabi::type
    {
//      if (not path_matches()) throw std::runtime_error("[BsonDecryptor]");
      if (not path_matches()) throw TodoError();
      auto do_paths_match =
        [&](const std::vector<std::string> &path) { return std::ranges::equal(path, m_current_path); };
      auto path_matches = std::ranges::find_if(metadata.paths(), do_paths_match);
      auto pos = std::distance(metadata.paths().begin(), path_matches);
      auto type_as_str = metadata.types.at(pos);
      
      if (type_as_str == "string") { return bsoncxx::v_noabi::type::k_string; }
      else if (type_as_str == "int") { return bsoncxx::v_noabi::type::k_int32; }
      else if (type_as_str == "long") { return bsoncxx::v_noabi::type::k_int64; }
      else if (type_as_str == "double") { return bsoncxx::v_noabi::type::k_double; }
      else { throw TodoError(); }
//      else { throw std::runtime_error("[BsonDecryptor]"); }
    }
    
    EncryptionService encryption_agent;
    PropertyMetadataStorage metadata;
    
   
    
    
    bsoncxx::document::value decrypt_deep(bsoncxx::document::view document)
    {
      return decrypt_object(document);
    }
    
    inline void push_path(std::string path_node) { m_current_path.emplace_back(std::move(path_node)); }
    
    inline void pop_path() { m_current_path.pop_back(); }
    
    bsoncxx::document::value decrypt_object(bsoncxx::document::view object)
    {
      static constexpr auto k_array_placeholder = "[0]";
      
      using namespace bsoncxx::builder::basic;
//      using namespace bsoncxx::builder:
      document result{};
      for (const auto &element: object)
      {
        bool was_object =
          not(element.key().front() == '0'
              or element.key().front() == '1'
              or element.key().front() == '2'
              or element.key().front() == '3');
        if (element.type() == bsoncxx::v_noabi::type::k_document)
        {
          if (element.get_document().view().empty())
          {
            result.append(kvp(element.key(), make_document()));
            continue;
          }
          push_path(was_object ? element.key().data() : k_array_placeholder);
          result.append(kvp(element.key(), decrypt_object(element.get_document().view())));
          pop_path();
        }
        else if (element.type() == bsoncxx::v_noabi::type::k_array)
        {
          auto arr = element.get_array();
          if (static_cast<bsoncxx::array::view>(arr).empty())
          {
            result.append(kvp(element.key(), make_array()));
            continue;
          }
          push_path(was_object ? element.key().data() : k_array_placeholder);
          result.append(kvp(element.key(), decrypt_array(arr)));
          pop_path();
        }
        else
        {
          push_path(was_object ? element.key().data() : k_array_placeholder);
          if (not(element.type() == bsoncxx::v_noabi::type::k_string
                  or element.type() == bsoncxx::v_noabi::type::k_int64
                  or element.type() == bsoncxx::v_noabi::type::k_oid
                  or element.type() == bsoncxx::v_noabi::type::k_binary
          )
            )
          {
            static_assert(static_cast<uint8_t>(bsoncxx::v_noabi::type::k_oid) == 7);
            
            std::cerr << "UNSUPPORTED TYPE : " << int(element.type()) << '\n';

//            throw std::runtime_error("TODO");
            throw TodoError();
          }
          if (path_matches())
          {
            auto type_tag = get_type_if_path_matches();
            auto encryption_tag = get_encryption_if_path_matches();
            if (type_tag == bsoncxx::v_noabi::type::k_string)
            {
              result.append(kvp(element.key(), decrypt_primitive_string(element.get_binary())));
            }
            else if (type_tag == bsoncxx::v_noabi::type::k_int64)
            {
              if (encryption_tag == EncryptionKind::AES)
              {
                result.append(kvp(element.key(), decrypt_int64_from_binary(element.get_binary())));
                
              }
              else if (encryption_tag == EncryptionKind::OPE) // NOLINT
              {
                result.append(kvp(element.key(), decrypt_int32_from_ope(element.get_int64())));
              }
            }
            
            else if (type_tag == bsoncxx::v_noabi::type::k_int32) {
              // type_tag zależny jest od property meta, to bardzo dobrze akurat :)
//              throw TodoError();
//              if (encryption_tag == EncryptionKind::)
                if (encryption_tag == EncryptionKind::AES) {
                  result.append(kvp(element.key(), decrypt_int64_from_binary(element.get_binary())));
                } else if (encryption_tag == EncryptionKind::OPE) {
                  result.append(kvp(element.key(), decrypt_int32_from_ope(element.get_int64())));
                }
            }
            else {
              std::cerr << static_cast<int>(type_tag) << '\n';
              throw TodoError(); }
//            else { throw std::runtime_error("[BsonDecryptor]"); }
          }
          else
          {
            result.append(kvp(element.key(), element.get_value()));
          }
          pop_path();
        }
      }
      return result.extract();
    }
    
    bsoncxx::types::b_string decrypt_primitive_string(bsoncxx::types::b_binary element)
    {
      std::cout << "[decrypt_primitive_string]\n";
      auto decrypted = encryption_agent.decrypt_AES(element);
      
      auto *leak = new auto(decrypted);
      return bsoncxx::types::b_string(*leak);
    }
    
    bsoncxx::types::b_int64 decrypt_int64_from_binary(bsoncxx::types::b_binary element)
    {
      std::string decrypted = encryption_agent.decrypt_AES(element);
      return {bytes_to_i64(decrypted)};
    }
    
    bsoncxx::types::b_int32 decrypt_int32_from_ope(bsoncxx::types::b_int64 element)
    {
      if (element < 0) throw TodoError();
//      if (element < 0) throw std::runtime_error("[BsonDecryptor]");
      return {encryption_agent.decrypt_uint64(element)};
    }
    
    bsoncxx::types::b_int64 decrypt_primitive_int64(EncryptionKind encryption_kind, bsoncxx::types::b_binary element)
    {
      std::cout << "[decrypt_primitive_int64\n";
      std::int64_t as_int;
      switch (encryption_kind)
      {
        case EncryptionKind::AES:
          as_int = [&]
          {
            std::string decrypted = encryption_agent.decrypt_AES(element);
            return bytes_to_i64(decrypted);
          }();
          break;
        case EncryptionKind::OPE:
          as_int = [&] -> int64_t
          {
//            return encryption_agent.decrypt_OPE(element); //TODO
            return 0;
          }();
          break;
      }
      return bsoncxx::types::b_int64(as_int);
    }
    
    bsoncxx::array::value decrypt_array(bsoncxx::array::view array)
    {
      static constexpr auto k_array_placeholder = "[0]";
//      throw TodoError();
      bsoncxx::builder::basic::array result{};
      for (const auto &element: array)
      {
        bool was_object = not(element.key().front() == '0'
                              or element.key().front() == '1'
                              or element.key().front() == '2'
                              or element.key().front() == '3');
        if (element.type() == bsoncxx::v_noabi::type::k_document)
        {
          if (element.get_document().view().empty())
          {
            result.append(bsoncxx::builder::basic::make_document());
            continue;
          }
          push_path(was_object ? element.key().data() : k_array_placeholder);
          result.append(decrypt_object(element.get_document().view()));
          pop_path();
        }
        else if (element.type() == bsoncxx::v_noabi::type::k_array)
        {
          bsoncxx::v_noabi::array::view arr = element.get_array();
          if (arr.empty())
          {
            result.append(bsoncxx::builder::basic::make_array());
            continue;
          }
          push_path(was_object ? element.key().data() : k_array_placeholder);
          result.append(decrypt_array(arr));
          pop_path();
        }
        else
        {
          push_path(was_object ? element.key().data() : k_array_placeholder);
          if (not(element.type() == bsoncxx::v_noabi::type::k_string
                  or element.type() == bsoncxx::v_noabi::type::k_int64
                  or element.type() == bsoncxx::v_noabi::type::k_oid
                  or element.type() == bsoncxx::v_noabi::type::k_binary
          )
            )
          {
            static_assert(static_cast<uint8_t>(bsoncxx::v_noabi::type::k_oid) == 7);
            
            std::cerr << "UNSUPPORTED TYPE : " << int(element.type()) << '\n';
            throw TodoError();
            
          }
          if (path_matches())
          {
            auto type_tag = get_type_if_path_matches();
            auto encryption_tag = get_encryption_if_path_matches();
            if (type_tag == bsoncxx::v_noabi::type::k_string)
            {
              result.append(decrypt_primitive_string(element.get_binary()));
            }
            else if (type_tag == bsoncxx::v_noabi::type::k_int64)
            {
              if (encryption_tag == EncryptionKind::AES)
              {
                result.append(decrypt_int64_from_binary(element.get_binary()));
              }
              else if (encryption_tag == EncryptionKind::OPE)
              {
                result.append(decrypt_int32_from_ope(element.get_int64()));
              }
            }
            else { throw TodoError(); }
//            else { throw std::runtime_error("BsonDecryptor"); }
          }
          else {
            result.append(element.get_value());
          }
          pop_path();
        }
      }
      return result.extract();
//      throw std::runtime_error("TODO");
//      throw std::runtime_error("TODO");
    }
  
  
};


#endif //MONGO_2_BSONDECRYPTOR_H
