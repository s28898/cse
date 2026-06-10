#include "../doctest/doctest.h"

#include "./CollectionServiceTest.h"

TEST_CASE("testing $set operator")
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

  auto insertCommand = jsonFromFile("../test_v3/data/find_predicates/set_operator_insert_cmd.json");

  collectionServicePtr->insertManyFromJson(insertCommand["documents"]);

  SUBCASE("updateOne $set enc")
  {

    const auto updateCommand = jsonFromFile(
      "../test_v3/data/find_predicates/set_operator/set_operator_update.json"
                                           );
    collectionServicePtr->updateManyFromJson(updateCommand["updates"][0]["q"], updateCommand["updates"][0]["u"]);


    const auto result = findAll(*collectionServicePtr);
    const auto expected = nlohmann::json::parse(
      R"([
      {
        "plain":"p1",
        "enc":"s1_new",
        "nested":{"enc2":"n1","plain2":"x"},
        "tags":["t1"],
        "items":[{"sku":"A","secretNote":"red"}]
      },
      {
        "plain":"p2",
        "enc":"s2",
        "nested":{"enc2":"n2","plain2":"x"},
        "tags":["t2"],
        "items":[{"sku":"B","secretNote":"blue"}]
      }
    ])"
                                               );

    REQUIRE(strip_oid(result) == expected);
  }

}
