#include "../doctest/doctest.h"

#include "./CollectionServiceTest.h"

TEST_CASE("testing ope out of bounds exception")
{

  constexpr auto schemaValidatorExtFilename = "../test_v3/data/range_predicates/range_predicates_validator.json";
  constexpr auto schemaValidatorTransformedOutputFilename = "../test_v3/data/range_predicates/output/range_predicates_validator_transformed.json";
  constexpr auto propertyMetadataStorageOutputFilename = "../test_v3/data/range_predicates/output/range_predicates_property_meta.json";

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

  generateKeys(*container.keyPolicyPtr(), "../dummy/context", true);

  const auto collectionServicePtr = container.collectionServicePtr();
  const auto databasePtr = container.databasePtr();

  const auto validatorTransformed = jsonFromFile(validatorTransformedFilename);
  overrideCollection(*databasePtr, collectionName, validatorTransformed);

  SUBCASE("insert rejects num > INT32_MAX")
  {
    const auto docsToInsert =
      jsonFromFile("../test_v3/data/range_predicates/range_predicates_insert_overflow.json");
    CHECK_THROWS(collectionServicePtr->insertManyFromJson(docsToInsert));
  }
  SUBCASE("find rejects threshold > INT32_MAX")
  {
    const auto findQuery = jsonFromFile("../test/data/range_predicates/range_predicates_find_overflow.json");
    CHECK_THROWS(collectionServicePtr->findManyFromJson(findQuery));
  }
}
