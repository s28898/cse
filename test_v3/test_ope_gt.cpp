#include "../doctest/doctest.h"

#include "./CollectionServiceTest.h"

TEST_CASE("testing $gt and $gte")
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



  const auto storage = PropertyMetadataStorage::from_file(propertyMetadataStorageOutputFilename);
  std::cout << "storage: " << storage.to_json() << '\n';


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


  const auto docsToInsert =jsonFromFile("../test_v3/data/range_predicates/range_predicates_insert.json");
  collectionServicePtr->insertManyFromJson(docsToInsert);

  SUBCASE("testing $gt operator")
  {
    const auto findQuery = jsonFromFile("../test_v3/data/range_predicates/range_predicates_find_gt.json");
    auto cursor = collectionServicePtr->findManyFromJson(findQuery);
    const auto result = jsonArrayFromCursor(cursor);

    const auto expected = nlohmann::json::parse(
      R"([
        { "plain": "c", "num": 30 },
        { "plain": "d", "num": 40 },
        { "plain": "e", "num": 50 }
      ])"
                                               );
    REQUIRE(strip_oid(result) == expected);
  }


  SUBCASE("testing $gte operator")
  {
    const auto findQuery = jsonFromFile("../test_v3/data/range_predicates/range_predicates_find_gte.json");
    auto cursor = collectionServicePtr->findManyFromJson(findQuery);
    const auto result = jsonArrayFromCursor(cursor);

    const auto expected = nlohmann::json::parse(
      R"([
        { "plain": "c", "num": 30 },
        { "plain": "d", "num": 40 },
        { "plain": "e", "num": 50 }
      ])"
                                               );
    REQUIRE(strip_oid(result) == expected);
  }
}
