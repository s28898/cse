#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_range_predicates_util.h"

TEST_CASE("testing $lt and $lte operator")
{
  OpeSimpleTest test = makeRangePredicatesTest("range_predicates");
  test.createCollection();
  const auto docs = fromFile("../test/data/range_predicates/range_predicates_insert.json");
  test.insertManyFromStatement(docs);
  
  SUBCASE("testing $lt operator")
  {
    std::string json_string = test.findManyFromFile("../test/data/range_predicates/range_predicates_find_lt.json");
    REQUIRE(!json_string.empty());
    
    const auto json = nlohmann::json::parse(json_string);
    const auto result = strip_oid(json["result"]);
    
    const auto expected = nlohmann::json::parse(
      R"([
        { "plain": "a", "num": 10 },
        { "plain": "b", "num": 20 }
      ])"
                                               );
    REQUIRE(result == expected);
  }
  
  SUBCASE("testing $lte operator")
  {
    std::string json_string = test.findManyFromFile("../test/data/range_predicates/range_predicates_find_lte.json");
    REQUIRE(!json_string.empty());
    
    const auto json = nlohmann::json::parse(json_string);
    const auto result = strip_oid(json["result"]);
    
    const auto expected = nlohmann::json::parse(
      R"([
        { "plain": "a", "num": 10 },
        { "plain": "b", "num": 20 }
      ])"
                                               );
    REQUIRE(result == expected);
  }
}