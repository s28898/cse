//
// Created by Bruno Kuś on 04/02/2026.
//
#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_find_predicates_util.h"

TEST_CASE("testing $elemMatch operator on arrays") {
  OpeSimpleTest test = makeFindPredicatesTest("find_elemMatch_array");
  test.createCollection();
  const auto docs = fromFile("../test/data/find_predicates/elemMatch_operator_array/elemMatch_operator_array_insert.json");
  test.insertManyFromStatement(docs);
  const auto findQuery = fromFile("../test/data/find_predicates/elemMatch_operator_array/elemMatch_operator_array_find.json");
  const std::string json_string = test.findManyFromStatement(findQuery);
  const auto json = nlohmann::json::parse(json_string);
  const auto expected = nlohmann::json::parse(
    R"([
    {
      "plain": "p2",
      "enc": "s2",
      "nested": { "enc2": "n2", "plain2": "x" },
      "tags": [],
      "items": [
        { "sku": "A", "secretNote": "red" },
        { "sku": "B", "secretNote": "blue" }
      ]
    }
  ])"
    );
  REQUIRE(strip_oid(json["result"]) == expected);
}