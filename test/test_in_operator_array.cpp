#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "./test_find_predicates_util.h"

TEST_CASE("testing in operator on array elements") {
  OpeSimpleTest test = makeFindPredicatesTest("find_in_operator_array");
  test.createCollection();
  
  const auto docs = fromFile("../test/data/find_predicates/in_operator_array/in_operator_array_insert.json");
  test.insertManyFromStatement(docs);
  
  std::string json_string = test.findManyFromFile("../test/data/find_predicates/in_operator_array/in_operator_array_find.json");
  const auto json = nlohmann::json::parse(json_string);
  
  const auto expected = nlohmann::json::parse(R"([
    { "plain": "p2", "enc": "s2", "nested": { "enc2": "n2", "plain2": "x" }, "tags": ["vip","eu"], "items": [] }
  ])");
  
  REQUIRE(strip_oid(json["result"]) == expected);
}