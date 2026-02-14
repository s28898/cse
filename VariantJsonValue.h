//
// Created by Bruno Kuś on 08/01/2026.
//

#ifndef MONGO_2_VARIANTJSONVALUE_H
#define MONGO_2_VARIANTJSONVALUE_H
#include <nlohmann/json.hpp>

struct VariantJsonValue
{
    using string_t = typename nlohmann::ordered_json::string_t;
    using boolean_t = typename nlohmann::ordered_json::boolean_t;
    using number_integer_t = typename nlohmann::ordered_json::number_integer_t;
    using number_unsigned_t = typename nlohmann::ordered_json::number_unsigned_t;
    using number_float_t = typename nlohmann::ordered_json::number_float_t;
//        using binary_t = typename nlohmann::ordered_json::binary_t;
    
    template<nlohmann::detail::value_t type_enum>
    using value_t = decltype([]
    {
      using namespace nlohmann::detail;
      if constexpr (type_enum == value_t::string) { return std::type_identity<string_t>{}; }
      else if constexpr (type_enum == value_t::boolean) { return std::type_identity<boolean_t>{}; }
      else if constexpr (type_enum == value_t::number_integer) { return std::type_identity<number_integer_t>{}; }
      else if constexpr (type_enum == value_t::number_unsigned) { return std::type_identity<number_unsigned_t>{}; }
      else if constexpr (type_enum == value_t::number_float) { return std::type_identity<number_float_t>{}; }
      else static_assert(false);
    }())::type;
    
    using variant = std::variant<
      string_t,
      boolean_t,
      number_integer_t,
      number_unsigned_t,
      number_float_t
    >;
    static variant make(nlohmann::ordered_json value) {
      return VariantJsonValue{value}.m_variant;
    }
    variant m_variant;
    VariantJsonValue(nlohmann::ordered_json value)
    {
      using namespace nlohmann;
      switch (value.type())
      {
        case detail::value_t::null:
          throw std::runtime_error("");
          break;
        case detail::value_t::object:
          throw std::runtime_error("");
          break;
        case detail::value_t::array:
          throw std::runtime_error("");
          break;
        case detail::value_t::string:
          m_variant =  value.get<value_t<detail::value_t::string>>();
          break;
        case detail::value_t::boolean:
          m_variant = value.get<value_t<detail::value_t::boolean>>();
          break;
        case detail::value_t::number_integer:
          m_variant = value.get<value_t<detail::value_t::number_integer>>();
          break;
        case detail::value_t::number_unsigned:
          m_variant = value.get<value_t<detail::value_t::number_unsigned>>();
          break;
        case detail::value_t::number_float:
          m_variant = value.get<value_t<detail::value_t::number_float>>();
          break;
        case detail::value_t::binary:
          throw std::runtime_error("");
          break;
        case detail::value_t::discarded:
          throw std::runtime_error("");
          break;
      }
    }
};
#endif //MONGO_2_VARIANTJSONVALUE_H
