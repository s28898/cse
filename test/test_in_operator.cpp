#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_find_predicates_util.h"
TEST_CASE("testing $in operator") {
  OpeSimpleTest test = makeFindPredicatesTest("find_in_operator");
  test.createCollection();
  
  const auto docs = fromFile("../test/data/find_predicates/in_operator/in_operator_insert.json");
  test.insertManyFromStatement(docs);
  const auto findQuery = fromFile("../test/data/find_predicates/in_operator/in_operator_find.json");
  const std::string json_string = test.findManyFromStatement(findQuery);
  const auto json = nlohmann::json::parse(json_string);
  
//  std::cout << "docs: " << std::setw(2) << docs << '\n';
//  std::cout << "query: " << std::setw(2) << findQuery << '\n';
//  std::cout << "result: " << std::setw(2) << json << '\n';
  
  const auto expected = nlohmann::json::parse(
    R"([
    { "plain": "p1", "enc": "secretA", "nested": { "enc2": "nA", "plain2": "x" }, "tags": [], "items": [] },
    { "plain": "p3", "enc": "secretC", "nested": { "enc2": "nC", "plain2": "z" }, "tags": [], "items": [] }
  ])"
    );
  REQUIRE(strip_oid(json["result"]) == expected);
}