#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "./test_find_predicates_util.h"

TEST_CASE("testing dot path") {
  OpeSimpleTest test = makeFindPredicatesTest("find_dotpath");
  test.createCollection();
  const auto docs = fromFile("../test/data/find_predicates/dotpath/dotpath_insert.json");
  test.insertManyFromStatement(docs);
  const auto findQuery = fromFile("../test/data/find_predicates/dotpath/dotpath_find.json");
  const std::string json_string =test.findManyFromStatement(findQuery);
  const auto json = nlohmann::json::parse(json_string);
  const auto expected = nlohmann::json::parse(
    R"([
    { "plain": "p2", "enc": "s2", "nested": { "enc2": "needle", "plain2": "b" }, "tags": [], "items": [] }
  ])"
    );
  REQUIRE(strip_oid(json["result"]) == expected);
}