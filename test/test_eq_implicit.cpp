#include "../doctest/doctest.h"

#include "../AbstractTest.h"


#include "test_find_predicates_util.h"

TEST_CASE("testing implicit equality")
{
  OpeSimpleTest test = makeFindPredicatesTest("find_eq_implicit");
  test.createCollection();
  
  auto const docs = fromFile("../test/data/find_predicates/eq_implicit/eq_implicit_insert.json");
  test.insertManyFromStatement(docs);
  
  auto const findQuery = fromFile("../test/data/find_predicates/eq_implicit/eq_implicit_find.json");
  
  std::string json_string = test.findManyFromStatement(findQuery);
  auto const json = nlohmann::json::parse(json_string);
  
  std::cout << std::setw(2) << "query: " << findQuery << '\n';
  std::cout << std::setw(2) << "result: " << json["result"] << '\n';
  auto const expected = nlohmann::json::parse(
    R"([
     { "plain": "p1", "enc": "secretA", "nested": { "enc2": "nA", "plain2": "x" }, "tags": ["t1"], "items": [] }
  ])"
    );
  REQUIRE(strip_oid(json["result"]) == expected);
}