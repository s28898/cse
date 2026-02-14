#include "../../doctest/doctest.h"

#include "../CollectionServiceTest.h"

TEST_CASE("testing $and, $or, $nor operator")
{
  
  constexpr auto schemaValidatorExtFilename = "../test_v3/data/find_predicates/find_predicates_validator_ext.json";
  constexpr auto schemaValidatorTransformedOutputFilename = "../test_v3/data/find_predicates/output/find_predicates_validator_transformed.json";
  constexpr auto propertyMetadataStorageOutputFilename = "../test_v3/data/find_predicates/output/find_predicates_property_meta.json";
  
  constexpr auto propertyMetadataStorageFilename = propertyMetadataStorageOutputFilename;
  constexpr auto validatorTransformedFilename = schemaValidatorTransformedOutputFilename;
  constexpr auto uri = "mongodb://localhost:27017";
  constexpr auto databaseName = "testdb_v3";
  constexpr auto collectionName = "and_or_nor_operator";
  
  processSchemaValidator
    (
      schemaValidatorExtFilename,
      schemaValidatorTransformedOutputFilename,
      propertyMetadataStorageOutputFilename
    );
  
  
  DependencyContainer container
    {
      propertyMetadataStorageFilename,
      uri,
      databaseName,
      collectionName
    };
  
  const auto collectionServicePtr = container.collectionServicePtr();
  const auto databasePtr = container.databasePtr();
  
  const auto validatorTransformed = jsonFromFile(validatorTransformedFilename);
  overrideCollection(*databasePtr, collectionName, validatorTransformed);
  
  const auto docsToInsert =
    jsonFromFile("../test_v2/data/find_predicates/and_or_nor_operator/and_or_nor_operator_insert.json");
  collectionServicePtr->insertManyFromJson(docsToInsert);
  
  const auto findQuery = jsonFromFile("../test_v2/data/find_predicates/and_or_nor_operator/and_or_nor_operator_find.json");
  collectionServicePtr->findManyFromJson(findQuery);
}