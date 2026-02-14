//
// Created by Bruno Kuś on 04/02/2026.
//
#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_find_predicates_util.h"

TEST_CASE("testing $eq operator")
{
  OpeSimpleTest test = makeFindPredicatesTest("find_eq_operator");
  test.createCollection();
  
  const auto docs = fromFile("../test/data/find_predicates/eq_operator/eq_operator_insert.json");
  test.insertManyFromStatement(docs);
  const auto findQuery = fromFile("../test/data/find_predicates/eq_operator/eq_operator_find.json");
  const std::string json_string = test.findManyFromStatement(findQuery);
  const auto json = nlohmann::json::parse(json_string);
//  std::cout << "docs: " << std::setw(2) << docs << '\n';
//  std::cout << "query: " << std::setw(2) << findQuery << '\n';
//  std::cout << "result: " << std::setw(2) << json << '\n';
  
  const auto expected = nlohmann::json::parse(
    R"([
    { "plain": "p2", "enc": "secretB", "nested": { "enc2": "nB", "plain2": "y" }, "tags": [], "items": [] }
  ])"
    );
  REQUIRE(strip_oid(json["result"]) == expected);
  
}