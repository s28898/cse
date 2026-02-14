#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_find_predicates_util.h"

TEST_CASE("testing dotpath on array of objects")
{
  OpeSimpleTest test = makeFindPredicatesTest("find_dotpath_array");
  test.createCollection();
  
  const auto docs = fromFile("../test/data/find_predicates/dotpath_array/dotpath_array_insert.json");
  test.insertManyFromStatement(docs);
  std::string json_string = test.findManyFromFile("../test/data/find_predicates/dotpath_array/dotpath_array_find.json");
  REQUIRE(not json_string.empty());
  const auto json = nlohmann::json::parse(json_string);
  const auto expected = nlohmann::json::parse(
    R"([
  {
    "plain": "p2",
    "enc": "s2",
    "nested": { "enc2": "n2", "plain2": "x" },
    "tags": [],
    "items": [
      { "sku": "A", "secretNote": "red" }
    ]
  },
  {
    "plain": "p3",
    "enc": "s3",
    "nested": { "enc2": "n3", "plain2": "x" },
    "tags": [],
    "items": [
      { "sku": "B", "secretNote": "red" }
    ]
  }
])"
                                             );
  REQUIRE(strip_oid(json["result"]) == expected);
  
}