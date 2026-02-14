#include "../doctest/doctest.h"
#include "../AbstractTest.h"

TEST_CASE("testing ope simple")
{
  OpeSimpleTest test = OpeSimpleTest::makeTest("ope_simple", "../test/data");

//  test.createCollection();
//  test.insertMany("../test/data/ope_simple/insert1.json");
//  test.findMany("../test/data/ope_simple/find1.json");
  
  test.createCollection();
  const auto docs = fromFile("../test/data/ope_simple/insert1.json");
  test.insertMany(docs);
  const auto &json_string = test.findManyFromFile("../test/data/ope_simple/find1.json");
  nlohmann::ordered_json json = nlohmann::json::parse(json_string);
  std::cout << json << '\n';
  
  REQUIRE(json["result"].size() == 3);
//  const auto doc1 = json["result"]["Bruno"];
//  const auto doc2 = json["result"]["Mirek"];
//  const auto doc3 = json["result"]["Alicja"];
  ordered_json doc1, doc2, doc3;
  for (const auto& doc : json["result"]) {
    if (doc["name"] == "Bruno") doc1 = doc;
    if (doc["name"] == "Mirek") doc2 = doc;
    if (doc["name"] == "Alicja") doc3 = doc;
  }
  
  REQUIRE(doc1["age"] == 10);
  REQUIRE(doc2["age"] == 50);
  REQUIRE(doc3["age"] == 18);
  
}