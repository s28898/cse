#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_find_predicates_util.h"

TEST_CASE("testing $not operator") {
  OpeSimpleTest test = makeFindPredicatesTest("find_not_operator");
  test.createCollection();
  const auto docs = fromFile("../test/data/find_predicates/not_operator/not_operator_insert.json");
  test.insertManyFromStatement(docs);
  const auto findQuery = fromFile ("../test/data/find_predicates/not_operator/not_operator_find.json");
  const std::string json_string = test.findManyFromStatement(findQuery);
  const auto json = nlohmann::json::parse(json_string);
  const auto expected = nlohmann::json::parse(
    R"([
    { "plain": "john", "enc": "secretA", "nested": { "enc2": "nX", "plain2": "x" }, "tags": [], "items": [] },
    { "plain": "kate", "enc": "secretA", "nested": { "enc2": "nZ", "plain2": "x" }, "tags": [], "items": [] }
  ])"
    );
  REQUIRE(strip_oid(json["result"]) == expected);
}