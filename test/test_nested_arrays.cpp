//
// Created by Bruno Kuś on 02/02/2026.
//
#include "../doctest/doctest.h"

#include "../AbstractTest.h"


TEST_CASE("testing nested arrays") {
  OpeSimpleTest test = OpeSimpleTest::makeTest("nested_arrays", "../test/data");
  
  test.createCollection();
  
  const auto statement = fromFile("../test/data/nested_arrays/insert1.json");
  
  test.insertManyFromStatement(statement);
  
  // muszę otworzyć sobie tą tablicę jako json...
  
//  bool queryFailed = false;
  std::string json_string;
//  try
//  {
    json_string = test.findManyFromFile("../test/data/nested_arrays/find1.json");
//  } catch (...) {
//    queryFailed = true;
//  }
//  REQUIRE(not queryFailed);
  nlohmann::ordered_json json = nlohmann::json::parse(json_string);
  const  nlohmann::ordered_json json_result = json["result"];
  std::cout << "json_result:\n" << json_result << '\n';
  
  const auto json_result_without_oid = [json_result = json_result] mutable -> nlohmann::ordered_json {
  for (auto& element : json_result) {element.erase("_id");}
  return json_result;
  }();
  
  std::cout << json_result_without_oid << '\n' << statement << '\n';
  REQUIRE(json_result_without_oid == statement);
  
}