#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "./test_find_predicates_util.h"

TEST_CASE("testing deletion") {
  constexpr auto collection_name = "set_operator_dotpath";
  OpeSimpleTest test = makeFindPredicatesTest(collection_name);
  test.createCollection();
  auto insertCommand = fromFile("../test/data/find_predicates/set_operator_insert_cmd.json");
  insertCommand["insert"] = collection_name;
  
  test.insertManyFromCommand(insertCommand);
  
  auto find_all = [&test, &collection_name] -> ordered_json
  {
    const auto json =
      nlohmann::json::parse(
        bsoncxx::to_json(
          test.runCommandFromStatement(
            {{"find", collection_name},
             {"filter", nlohmann::json::object()}}
                                      ),
          bsoncxx::ExtendedJsonMode::k_relaxed
                        )
                           );
    std::cerr << "findAllJson\n" << json << '\n';
    return strip_oid(json["result"]);
  };
  
  SUBCASE("deleteOne by plain") {
    const auto deleteCommand = [&collection_name] ->ordered_json
    {
      auto command = fromFile("../test/data/find_predicates/deletion/deletion_delete_one_cmd.json");
      command["delete"] = collection_name;
      return command;
    }();
    test.deleteFromCommand(deleteCommand);
    const auto result = find_all();
    const auto expected = nlohmann::json::parse(R"([
  {
    "plain":"p2",
    "enc":"s2",
    "nested":{"enc2":"n2","plain2":"x"},
    "tags":["t2"],
    "items":[{"sku":"B","secretNote":"blue"}]
  }
])");
    REQUIRE(result == expected);
  }
  SUBCASE("deleteMany by enc (AES filter transformation)")
  {
    const auto deleteCommand = [&collection_name] -> ordered_json
    {
      auto command = fromFile("../test/data/find_predicates/deletion/deletion_delete_many_cmd.json");
      command["delete"] = collection_name;
      return command;
    }();
    test.deleteFromCommand(deleteCommand);
    const auto result = find_all();
    const auto expected = nlohmann::json::parse(R"([
      {
        "plain":"p1",
        "enc":"s1",
        "nested":{"enc2":"n1","plain2":"x"},
        "tags":["t1"],
        "items":[{"sku":"A","secretNote":"red"}]
      }
    ])");
    REQUIRE(result == expected);
  }
}