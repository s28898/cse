#include "../doctest/doctest.h"

#include "./CollectionServiceTest.h"

TEST_CASE("testing $in operator array")
{
  
  constexpr auto schemaValidatorExtFilename = "../test_v3/data/find_predicates/find_predicates_validator_ext.json";
  constexpr auto schemaValidatorTransformedOutputFilename = "../test_v3/data/find_predicates/output/find_predicates_validator_transformed.json";
  constexpr auto propertyMetadataStorageOutputFilename = "../test_v3/data/find_predicates/output/find_predicates_property_meta.json";
  
  constexpr auto propertyMetadataStorageFilename = propertyMetadataStorageOutputFilename;
  constexpr auto validatorTransformedFilename = schemaValidatorTransformedOutputFilename;
  constexpr auto keyPolicyFilename = "../test_v3/data/find_predicates/find_predicates_key_policy.json";
  constexpr auto uri = "mongodb://localhost:27017";
  constexpr auto databaseName = "testdb";
  constexpr auto collectionName = "testcol";
  
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
  
  generate_dummy_keys(*container.keyPolicyPtr(), "../dummy/context", true);
  
  const auto collectionServicePtr = container.collectionServicePtr();
  const auto databasePtr = container.databasePtr();
  
  const auto validatorTransformed = jsonFromFile(validatorTransformedFilename);
  overrideCollection(*databasePtr, collectionName, validatorTransformed);
  
  const auto docsToInsert =
    jsonFromFile("../test_v3/data/find_predicates/in_operator_array/in_operator_array_insert.json");
  collectionServicePtr->insertManyFromJson(docsToInsert);
  
  const auto findQuery = jsonFromFile("../test_v3/data/find_predicates/in_operator_array/in_operator_array_find.json");
  InputRangeOf<bsoncxx::document::value> auto cursor = collectionServicePtr->findManyFromJson(findQuery);
  
  const auto result = jsonArrayFromCursor(cursor);
  const auto expected = nlohmann::json::parse(R"([
    { "plain": "p2", "enc": "s2", "nested": { "enc2": "n2", "plain2": "x" }, "tags": ["vip","eu"], "items": [] }
  ])");
  
  REQUIRE(strip_oid(result) == expected);
}