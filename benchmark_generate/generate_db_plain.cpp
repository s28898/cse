
#include <mongocxx/database.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include "../bson_util.h"
#include <nlohmann/json.hpp>
#include <random>





int main()
{
  constexpr auto collectionName = "collection_plain";
  mongocxx::instance instance{};
  mongocxx::client client{mongocxx::uri{"mongodb://localhost:27091"}};
  mongocxx::database database = client["benchmark_plain"];

  const auto validator = json_to_bson_document(jsonFromFile("../benchmark_generate/benchmark_db_plain_validator.json"));

  if (not database.has_collection(collectionName))
  {
    database.create_collection(collectionName, validator.view());
  }

  mongocxx::collection collection = database[collectionName];

  constexpr auto documentTotal = 500'000;
  constexpr auto distinctSecretTotal = 1'000;

  const auto secrets =
  std::views::iota(0, distinctSecretTotal)
  | std::views::transform([](auto num) -> std::string { return "secret_" + std::to_string(num); })
  | std::ranges::to<std::vector>();


  std::mt19937_64 rng1{123456789};
  std::uniform_int_distribution<std::size_t> pick_secret(0, distinctSecretTotal - 1);
  std::uniform_int_distribution<std::int32_t> pick_num(0, std::numeric_limits<std::int32_t>::max());


  for (int i = 0; i < documentTotal; ++i)
  {
    bsoncxx::builder::basic::document doc;
    doc.append(bsoncxx::builder::basic::kvp("secret", secrets.at(pick_secret(rng1))));
    doc.append(bsoncxx::builder::basic::kvp("num", pick_num(rng1)));
    doc.append(bsoncxx::builder::basic::kvp("payload", std::string(256, 'x')));
    collection.insert_one(doc.view());
  }

  bsoncxx::builder::basic::document idx1;
  idx1.append(bsoncxx::builder::basic::kvp("secret", 1));
  collection.create_index(idx1.view());

  bsoncxx::builder::basic::document idx2;
  idx2.append(bsoncxx::builder::basic::kvp("num", 1));
  collection.create_index(idx2.view());
}
