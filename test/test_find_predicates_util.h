//
// Created by Bruno Kuś on 04/02/2026.
//

#ifndef MONGO_2_TEST_FIND_PREDICATES_UTIL_H
#define MONGO_2_TEST_FIND_PREDICATES_UTIL_H

#include "../AbstractTest.h"

inline nlohmann::json strip_oid(nlohmann::json arr) {
  for (auto& element : arr) element.erase("_id");
  return arr;
}

inline auto makeFindPredicatesTest(std::string collection_name) -> OpeSimpleTest
{
  const std::string_view path = "../test/data/find_predicates";
  const auto input_file_schema_validator_ext = [path] -> std::string
  {
    std::stringstream ss;
    ss << path << '/' << "find_predicates_validator.json";
    return ss.str();
  }
  ();
  const auto output_file_schema_validator_transformed = [path] -> std::string
  {
    std::stringstream ss;
    ss << path << '/' << "output" << '/' << "find_predicates_validator_transformed.json";
    return ss.str();
  }
  ();
  const auto output_file_property_meta = [path] -> std::string
  {
    std::stringstream ss;
    ss << path << '/' << "output" << '/' << "find_predicates_property_meta.json";
    return ss.str();
  }
  ();
  auto encryption_agent = [] -> EncryptionService
  {
    EncryptionService encryption_agent;
    encryption_agent.load("../secret/context.bin");
    return encryption_agent;
  }();
  std::string_view uri{"mongodb://localhost:27017"};
  std::string_view dbname{"testdb"};
  return {
    input_file_schema_validator_ext,
    output_file_schema_validator_transformed,
    output_file_property_meta,
    collection_name,
    std::move(encryption_agent),
    uri,
    dbname
  };
}


#endif //MONGO_2_TEST_FIND_PREDICATES_UTIL_H
