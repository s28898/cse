
//
// Created by Bruno Kuś on 08/01/2026.
//
#include "bson_util.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <nlohmann/json.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/types/bson_value/view.hpp>
#include <iostream>
#include <fstream>

auto json_to_bson_array(const ordered_json& j) -> bsoncxx::builder::basic::array
{
  bsoncxx::builder::basic::array arr;
  for (const auto& el : j)
  {
    if (el.is_object())
    {
      arr.append(json_to_bson_document(el).extract());
    }
    else if (el.is_array())
    {
      arr.append(json_to_bson_array(el).extract());
    }
    else if (el.is_string())
    {
      arr.append(el.get<std::string>());
    }
    else if (el.is_boolean())
    {
      arr.append(el.get<bool>());
    }
    else if (el.is_number_integer())
    {
      // Preserve int32/int64 depending on magnitude
      const auto v = el.get<long long>();
      if (v >= std::numeric_limits<std::int32_t>::min() && v <= std::numeric_limits<std::int32_t>::max())
        arr.append(static_cast<std::int32_t>(v));
      else
        arr.append(static_cast<std::int64_t>(v));
    }
    else if (el.is_number_unsigned())
    {
      const auto v = el.get<unsigned long long>();
      if (v <= static_cast<unsigned long long>(std::numeric_limits<std::int32_t>::max()))
        arr.append(static_cast<std::int32_t>(v));
      else if (v <= static_cast<unsigned long long>(std::numeric_limits<std::int64_t>::max()))
        arr.append(static_cast<std::int64_t>(v));
      else
        arr.append(static_cast<double>(v));
    }
    else if (el.is_number_float())
    {
      arr.append(el.get<double>());
    }
    else if (el.is_null())
    {
      arr.append(bsoncxx::types::b_null{});
    }
    else
    {
      // Fallback: store as string representation
      arr.append(el.dump());
    }
  }
  return arr;
}

auto json_to_bson_document(const ordered_json& j) -> bsoncxx::builder::basic::document
{
  bsoncxx::builder::basic::document doc;
  using bsoncxx::builder::basic::kvp;
  
  for (const auto& [k, v] : j.items())
  {
    if (v.is_object())
    {
      doc.append(kvp(k, json_to_bson_document(v).extract()));
    }
    else if (v.is_array())
    {
      doc.append(kvp(k, json_to_bson_array(v).extract()));
    }
    else if (v.is_string())
    {
      doc.append(kvp(k, v.get<std::string>()));
    }
    else if (v.is_boolean())
    {
      doc.append(kvp(k, v.get<bool>()));
    }
    else if (v.is_number_integer())
    {
      const auto n = v.get<long long>();
      if (n >= std::numeric_limits<std::int32_t>::min() && n <= std::numeric_limits<std::int32_t>::max())
        doc.append(kvp(k, static_cast<std::int32_t>(n)));
      else
        doc.append(kvp(k, static_cast<std::int64_t>(n)));
    }
    else if (v.is_number_unsigned())
    {
      const auto n = v.get<unsigned long long>();
      if (n <= static_cast<unsigned long long>(std::numeric_limits<std::int32_t>::max()))
        doc.append(kvp(k, static_cast<std::int32_t>(n)));
      else if (n <= static_cast<unsigned long long>(std::numeric_limits<std::int64_t>::max()))
        doc.append(kvp(k, static_cast<std::int64_t>(n)));
      else
        doc.append(kvp(k, static_cast<double>(n)));
    }
    else if (v.is_number_float())
    {
      doc.append(kvp(k, v.get<double>()));
    }
    else if (v.is_null())
    {
      doc.append(kvp(k, bsoncxx::types::b_null{}));
    }
    else
    {
      doc.append(kvp(k, v.dump()));
    }
  }
  
  return doc;
}

auto append_json_kvp(bsoncxx::builder::basic::document& out, const std::string& key, const ordered_json& v) -> void
{
  using bsoncxx::builder::basic::kvp;
  
  if (v.is_object())
  {
    out.append(kvp(key, json_to_bson_document(v).extract()));
  }
  else if (v.is_array())
  {
    out.append(kvp(key, json_to_bson_array(v).extract()));
  }
  else if (v.is_string())
  {
    out.append(kvp(key, v.get<std::string>()));
  }
  else if (v.is_boolean())
  {
    out.append(kvp(key, v.get<bool>()));
  }
  else if (v.is_number_integer())
  {
    const auto n = v.get<long long>();
    if (n >= std::numeric_limits<std::int32_t>::min() && n <= std::numeric_limits<std::int32_t>::max())
      out.append(kvp(key, static_cast<std::int32_t>(n)));
    else
      out.append(kvp(key, static_cast<std::int64_t>(n)));
  }
  else if (v.is_number_unsigned())
  {
    const auto n = v.get<unsigned long long>();
    if (n <= static_cast<unsigned long long>(std::numeric_limits<std::int32_t>::max()))
      out.append(kvp(key, static_cast<std::int32_t>(n)));
    else if (n <= static_cast<unsigned long long>(std::numeric_limits<std::int64_t>::max()))
      out.append(kvp(key, static_cast<std::int64_t>(n)));
    else
      out.append(kvp(key, static_cast<double>(n)));
  }
  else if (v.is_number_float())
  {
    out.append(kvp(key, v.get<double>()));
  }
  else if (v.is_null())
  {
    out.append(kvp(key, bsoncxx::types::b_null{}));
  }
  else
  {
    out.append(kvp(key, v.dump()));
  }
}

int64_t bytes_to_i64(std::string_view plain) {
  if (plain.size() != sizeof(int64_t)) {
    throw std::runtime_error("bytes_to_i64: expected 8 bytes");
  }
  int64_t x{};
  std::memcpy(&x, plain.data(), sizeof(x));  // safe for alignment/aliasing
  return x;
}
void print_binstr(std::string& s) {
  std::cout << "[print_binstr]\n";
  for (auto byte : s) {
    std::cout << (byte == '\0' ? 0 : byte) << ' ';
  }
  std::cout << '\n';
}

ordered_json fromFile(const std::string_view filename)
{
  ordered_json ordered_json;
  std::ifstream input_file(filename.data());
  input_file >> ordered_json;
  return ordered_json;
}
void toFile(const std::string_view filename, const ordered_json &document)
{
  constexpr static auto k_indentation = 2;
  std::cout << "Saving to file:" << filename << '\n';
  std::cout << "document.size = " << document.size() << '\n';
  std::cout << std::setw(k_indentation) << document << '\n';
  std::ofstream output_file(filename.data());
  output_file << std::setw(k_indentation) << document;
}

void debug_document(bsoncxx::document::view view) {
  std::cout << "[debug_document]\n";
  
  for (const auto& element: view) {
    std::cout << "key : " << element.key() << "\n"
              << "type==k_binary: " << (element.type() == bsoncxx::v_noabi::type::k_binary) << '\n'
              << "type: " << int(element.type())
              << '\n';
  }
}



ordered_json jsonFromFile(std::string_view filename) { return fromFile(filename); }
void jsonToFile(std::string_view filename, const ordered_json &document) { return toFile(filename, document); }

