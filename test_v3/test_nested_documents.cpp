#include "../doctest/doctest.h"

#include "./CollectionServiceTest.h"

TEST_CASE("testing nested documents")
{
  constexpr auto schemaValidatorExtFilename = "../test_v3/data/nested_documents/nested_documents_validator.json";
  constexpr auto schemaValidatorTransformedOutputFilename = "../test_v3/data/nested_documents/output/nested_documents_validator_transformed.json";
  constexpr auto propertyMetadataStorageOutputFilename = "../test_v3/data/nested_documents/output/nested_documents_propert_meta.json";

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

  const auto docsToInsert = jsonFromFile("../test_v3/data/nested_documents/insert1.json");
  collectionServicePtr->insertManyFromJson(docsToInsert);

  const auto findQuery = jsonFromFile("../test_v3/data/nested_documents/find1.json");
  InputRangeOf<bsoncxx::document::value> auto cursor = collectionServicePtr->findManyFromJson(findQuery);

  const auto result = jsonArrayFromCursor(cursor);
  const auto expected = docsToInsert;
  REQUIRE(strip_oid(result) == expected);
}
