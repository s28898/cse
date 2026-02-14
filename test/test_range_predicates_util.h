#ifndef MONGO_2_TEST_RANGE_PREDICATES_UTIL_H
#define MONGO_2_TEST_RANGE_PREDICATES_UTIL_H
   
    #include "../AbstractTest.h"
    
    
    inline nlohmann::json strip_oid(nlohmann::json arr) {
  for (auto& element : arr) element.erase("_id");
  return arr;
}
  auto run_find_and_expect = [](OpeSimpleTest& test, const char* find_file, const char* expected_json_literal)
  {
    std::string json_string = test.findManyFromFile(find_file);
    REQUIRE(!json_string.empty());

    const auto json = nlohmann::json::parse(json_string);
    const auto got = strip_oid(json["result"]);

    const auto expected = nlohmann::json::parse(expected_json_literal);
    return got == expected;
  };
inline auto makeRangePredicatesTest(std::string collection_name) -> OpeSimpleTest
{
  const std::string_view path = "../test/data/range_predicates";
  const auto input_file_schema_validator_ext = [path] -> std::string {
    std::stringstream ss;
    ss << path << '/' << "range_predicates_validator.json";
    return ss.str();
  }();
  const auto output_file_schema_validator_transformed = [path] -> std::string {
    std::stringstream ss;
    ss << path << '/' << "output" << '/' << "range_predicates_validator_transformed.json";
    return ss.str();
  }();
  const auto output_file_property_meta = [path] -> std::string {
    std::stringstream ss;
    ss << path << '/' << "output" << '/' << "range_predicates_property_meta.json";
    return ss.str();
  }();
  auto encryption_agent = [] -> EncryptionService {
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


    #endif