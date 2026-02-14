//
// Created by Bruno Kuś on 04/02/2026.
//

#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_find_predicates_util.h"

TEST_CASE("testing $and, $or, $nor operator") {
  OpeSimpleTest test = makeFindPredicatesTest("find_and_or_nor_operator");
  test.createCollection();
  const auto docs = fromFile("../test/data/find_predicates/and_or_nor_operator/and_or_nor_operator_insert.json");
  test.insertManyFromStatement(docs);
  const auto findQuery = fromFile("../test/data/find_predicates/and_or_nor_operator/and_or_nor_operator_find.json");
  const std::string json_string = test.findManyFromStatement(findQuery);
  const auto json = nlohmann::json::parse(json_string);
  const auto expected = nlohmann::json::parse(
    R"([
    { "plain": "john", "enc": "secretA", "nested": { "enc2": "nX", "plain2": "x" }, "tags": [], "items": [] }
  ])"
    );
  REQUIRE(strip_oid(json["result"]) == expected);
  
}