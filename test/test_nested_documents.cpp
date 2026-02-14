#include "../doctest/doctest.h"
#include "../AbstractTest.h"

TEST_CASE("testing nested documents") {
  OpeSimpleTest test = OpeSimpleTest::makeTest("nested_documents", "../test/data");
  test.createCollection();
  
  const auto statement = fromFile("../test/data/nested_documents/insert1.json");
  test.insertManyFromStatement(statement);
  
  std::string json_string = test.findManyFromFile("../test/data/nested_documents/find1.json");
  const auto json = nlohmann::json::parse(json_string);
  
  const auto json_result_without_oid = [json_result = json["result"]] mutable {
    for (auto& element : json_result) { element.erase("_id");}
    return json_result;
  }();
  
  REQUIRE(json_result_without_oid == statement);
}