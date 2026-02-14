#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_range_predicates_util.h"

TEST_CASE("testing $gt operator")
{
  OpeSimpleTest test = makeRangePredicatesTest("range_predicates");
  test.createCollection();
  const auto docs = fromFile("../test/data/range_predicates/range_predicates_insert.json");
  test.insertManyFromStatement(docs);
  
  std::string json_string = test.findManyFromFile("../test/data/range_predicates/range_predicates_find_gt.json");
  REQUIRE(!json_string.empty());
  
  const auto json = nlohmann::json::parse(json_string);
  const auto result = strip_oid(json["result"]);
  
  const auto expected = nlohmann::json::parse(
    R"([
        { "plain": "c", "num": 30 },
        { "plain": "d", "num": 40 },
        { "plain": "e", "num": 50 }
      ])"
                                             );
  REQUIRE(result == expected);
}