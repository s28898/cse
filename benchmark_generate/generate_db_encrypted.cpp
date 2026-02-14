
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include "../bson_util.h"
#include <nlohmann/json.hpp>
#include <random>

#include "../test_v3/CollectionServiceTest.h"

int main()
{
  mongocxx::instance instance{};
  
  constexpr auto uri = "mongodb://localhost:27092";
  constexpr auto databaseName = "benchmark_encrypted";
  constexpr auto collectionName = "collection_encrypted";


//  const auto validator = json_to_bson_document(jsonFromFile("../benchmark_generate/benchmark_db_plain_validator.json"));
  constexpr auto schemaValidatorExtFilename = "../benchmark_generate/benchmark_db_encrypted_validator.json";
  constexpr auto schemaValidatorTransformedOutputFilename = "../benchmark_generate/output/benchmark_db_encrypted_validator_transformed.json";
  constexpr auto propertyMetadataStorageOutputFilename = "../benchmark_generate/output/benchmark_db_encrypted_property_meta.json";
  constexpr auto propertyMetadataStorageFilename = propertyMetadataStorageOutputFilename;
  constexpr auto validatorTransformedFilename = schemaValidatorTransformedOutputFilename;
  
  constexpr auto keyPolicyFilename = "../benchmark_generate/benchmark_db_encrypted_key_policy.json";
  
  processSchemaValidator
    (
      schemaValidatorExtFilename,
      schemaValidatorTransformedOutputFilename,
      propertyMetadataStorageOutputFilename
    );
  
  DependencyContainer container
    {
      propertyMetadataStorageFilename,
      keyPolicyFilename,
      uri,
      databaseName,
      collectionName
    };
  
  CollectionServiceV3& collectionService = *container.collectionServicePtr();
  auto &database = *container.databasePtr();
  
  if (not database.has_collection(collectionName))
  {
    const auto validatorTransformed = bsoncxx::from_json(jsonFromFile(validatorTransformedFilename).dump());
    database.create_collection(collectionName, validatorTransformed.view());
  }
  
  std::cout << "estimated: " << database[collectionName].estimated_document_count();
  
  constexpr auto documentTotal = 500'000;
  constexpr auto distinctSecretTotal = 1'000;

  const auto secrets =
    std::views::iota(0, distinctSecretTotal)
    | std::views::transform([](auto num) -> std::string { return "secret_" + std::to_string(num); })
    | std::ranges::to<std::vector>();


  std::mt19937_64 rng{123456789};
  std::uniform_int_distribution<std::size_t> pick_secret(0, distinctSecretTotal - 1);
  std::uniform_int_distribution<std::int32_t> pick_num(0, std::numeric_limits<std::int32_t>::max());

  for (int i = 0; i < documentTotal; ++i)
  {
    bsoncxx::builder::basic::document doc;
    doc.append(bsoncxx::builder::basic::kvp("secret", secrets.at(pick_secret(rng))));
    doc.append(bsoncxx::builder::basic::kvp("num", pick_num(rng)));
    doc.append(bsoncxx::builder::basic::kvp("payload", std::string(256, 'x')));
    collectionService.insert_one(doc.view());
  }

    mongocxx::collection collection = database[collectionName];

  bsoncxx::builder::basic::document idx1;
  idx1.append(bsoncxx::builder::basic::kvp("secret", 1));
  collection.create_index(idx1.view());

  bsoncxx::builder::basic::document idx2;
  idx2.append(bsoncxx::builder::basic::kvp("num", 1));
  collection.create_index(idx2.view());
}