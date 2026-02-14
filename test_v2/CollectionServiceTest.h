//
// Created by Bruno Kuś on 09/02/2026.
//

#ifndef MONGO_2_COLLECTIONSERVICETEST_H
#define MONGO_2_COLLECTIONSERVICETEST_H


//inline createCollection() {
//
//}


#include <nlohmann/json_fwd.hpp>

#include <mongocxx/database.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include <bsoncxx/json.hpp>

//#include "SchemaValidatorTransformer.h"
#include "../doctest/doctest.h"
#include "../CollectionService.h"
//#include "../CollectionService.cpp"
#include "../DummyKeyProvider.h"
#include "../v2/EncryptionService_v2.h"
#include "../v2/StatementTransformerV2.h"
#include "../v2/SchemaValidatorTransformerV2.h"

using namespace std::literals::string_view_literals;


inline
auto overrideCollection(
  mongocxx::database &db,
  std::string_view collectionName,
  const nlohmann::ordered_json &validatorTransformed
) -> void
{
  if (db.has_collection(collectionName.data())) { db.collection(collectionName.data()).drop(); }
  db.create_collection(collectionName, bsoncxx::from_json(validatorTransformed.dump()));
}



inline
auto processSchemaValidator(
  std::string_view schemaValidatorExtFilename,
  std::string_view schemaValidatorTransformedOutputFilename,
  std::string_view propertyMetadataStorageOutputFilename
) -> void
{
  const auto schemaValidatorExt = jsonFromFile(schemaValidatorExtFilename);
  const auto [validatorTransformed, propertyMetadata] = [&schemaValidatorExt] -> auto
  {
    return SchemaValidatorTransformerV2{}.transform_schema_validator(schemaValidatorExt);
  }();
  jsonToFile(schemaValidatorTransformedOutputFilename, validatorTransformed);
  jsonToFile(propertyMetadataStorageOutputFilename, propertyMetadata.to_json());
}



struct DependencyContainer
{
   
    DependencyContainer(
      std::string_view propertyMetadataStorageFilename,
      std::string_view uri,
      std::string_view databaseName,
      std::string_view collectionName
    )
    : m_clientPtr{std::make_shared<mongocxx::client>(mongocxx::uri{uri.data()})}
    , m_databasePtr{std::make_shared<mongocxx::database>(m_clientPtr->database(databaseName.data()))}
    , m_dummyKeyProviderPtr{std::make_shared<DummyKeyProvider>()}
    , m_encryptionServicePtr{std::make_shared<EncryptionServiceV2>(m_dummyKeyProviderPtr)}
    , m_propertyMetadataStoragePtr
    {
        [&propertyMetadataStorageFilename] -> std::shared_ptr<PropertyMetadataStorage>
        {
          auto propertyMetadataStoragePtr = std::make_shared<PropertyMetadataStorage>();
          *propertyMetadataStoragePtr = PropertyMetadataStorage::from_file(propertyMetadataStorageFilename);
          return propertyMetadataStoragePtr;
        }()
    }
    , m_collectionServicePtr{ std::make_shared<CollectionService>(m_databasePtr, std::string(collectionName), m_encryptionServicePtr, m_propertyMetadataStoragePtr)}
    {};
    
    auto collectionServicePtr() const -> std::shared_ptr<CollectionService>
    {
      return m_collectionServicePtr;
    }
    
    auto databasePtr() const -> std::shared_ptr<mongocxx::database>
    {
      return m_databasePtr;
    }
    private:
    std::shared_ptr<mongocxx::client> m_clientPtr;
    std::shared_ptr<mongocxx::database> m_databasePtr;
    std::shared_ptr<EncryptionServiceV2> m_encryptionServicePtr;
    std::shared_ptr<CollectionService> m_collectionServicePtr;
    std::shared_ptr<PropertyMetadataStorage> m_propertyMetadataStoragePtr;
    std::shared_ptr<DummyKeyProvider> m_dummyKeyProviderPtr;
};


//TEST_CASE("new test")
//{
//
//  constexpr auto collectionName = "todo"sv;
//  constexpr auto propertyMetadataStorageFilename = "todo"sv;
//  constexpr auto validatorExtFilename = "todo"sv;
//  constexpr auto findCommandFilename = "todo"sv;
//  constexpr auto uri = "todo"sv;
//  constexpr auto databaseName = "test"sv;
//  constexpr auto schemaValidatorExtFilename = "todo"sv;
//  constexpr auto schemaValidatorOutputFilename = "todo"sv;
//  constexpr auto propertyMetadataStorageOutputFilename = "todo"sv;
//  constexpr auto validatorTransformedFilename = "todo"sv;
//  constexpr auto insertCommandFilename = "todo"sv;
//
//  processSchemaValidator(
//    schemaValidatorExtFilename,
//    schemaValidatorOutputFilename,
//    propertyMetadataStorageOutputFilename
//                               );
//
//  DependencyContainer container{
//    propertyMetadataStorageFilename,
//    uri,
//    databaseName,
//    collectionName
//  };
//
//  auto collectionServicePtr = container.collectionServicePtr();
//  auto databasePtr = container.databasePtr();
//
//  const auto validatorTransformed = jsonFromFile(validatorTransformedFilename);
//  ::overrideCollection(*databasePtr, collectionName, validatorTransformed);
//
//  const auto insertCommand = jsonFromFile(insertCommandFilename);
//  const auto findCommand = jsonFromFile(findCommandFilename);
//  collectionServicePtr->insertManyFromJson(insertCommand);
//
//  std::ranges::range<bsoncxx::document::value> auto foundDocs = collectionServicePtr->findManyFromCommand(findCommand);
//  // zbierz do jednego obiektu, aczkolwiek coś mi śmierdzą sygnatury tutaj... :(
//  nlohmann::json result(foundDocs);
//  const auto expected = nlohmann::json::object();
//  REQUIRE(result == expected);
//
//}


#endif //MONGO_2_COLLECTIONSERVICETEST_H

//struct DatabaseSetup
//{
//    DatabaseSetup(std::string_view uri, std::string_view databaseName)
//      : m_client{mongocxx::uri{uri.data()}},
//        m_databasePtr{std::make_shared<mongocxx::database>(m_client.database(databaseName.data()))} {}
//
//    mongocxx::client m_client;
//    std::shared_ptr<mongocxx::database> m_databasePtr;
//
//
//    auto databasePtr() const -> std::shared_ptr<mongocxx::database>
//    {
//      return m_databasePtr;
//    }
//
//    auto overrideCollection(std::string_view collectionName, const nlohmann::ordered_json &validator) -> void
//    {
//      return ::overrideCollection(*m_databasePtr, collectionName, validator);
//    }
//};

//struct DatabaseService {
//    DatabaseService(std::string_view uri, std::string_view databaseName)
//    : m_client{mongocxx::uri{uri.data()}}
//    , m_database(m_client.database(databaseName.data()))
//    {}
//    mongocxx::client m_client;
//    mongocxx::database m_database;
//};

//inline auto makePropertyMetadataStoragePtr(std::string_view filename) -> std::shared_ptr<PropertyMetadataStorage>
//{
//  auto propertyMetadataStoragePtr = std::make_shared<PropertyMetadataStorage>();
//  *propertyMetadataStoragePtr = PropertyMetadataStorage::from_file(filename);
//  return propertyMetadataStoragePtr;
//}
//
//inline auto makeEncryptionServicePtr() -> std::shared_ptr<EncryptionServiceV2>
//{
//  return std::make_shared<EncryptionServiceV2>(std::make_shared<DummyKeyProvider>());
//}

//inline auto makeCollectionService(
//  std::shared_ptr<mongocxx::database> databasePtr,
//  std::string_view collectionName,
//  std::string_view propertyMetadataStorageFilename,
//  std::shared_ptr<EncryptionServiceV2> encryptionServicePtr
//) -> CollectionService
//{
//  auto a_databasePtr = databasePtr;
//  auto a_collectionName = std::string(collectionName);
////  auto a_encryptionServicePtr = makeEncryptionServicePtr();
//  auto a_encryptionServicePtr = encryptionServicePtr;
//  auto a_propertyMetadataStoragePtr = makePropertyMetadataStoragePtr(propertyMetadataStorageFilename);
//  return {
//    std::move(databasePtr),
//    std::move(a_collectionName),
//    std::move(a_encryptionServicePtr),
//    std::move(a_propertyMetadataStoragePtr)
//  };
//}


//struct DependencyContainer
//{
//
//    DependencyContainer(
//      std::string_view propertyMetadataStorageFilename,
//      std::string_view uri,
//      std::string_view databaseName,
//      std::string_view collectionName
//    )
//      : m_clientPtr{std::make_shared<mongocxx::client>(mongocxx::uri{uri.data()})}
//      , m_databasePtr{std::make_shared<mongocxx::database>(m_clientPtr->database(databaseName.data()))}
//      , m_dummyKeyProviderPtr{std::make_shared<DummyKeyProvider>()}
//      , m_encryptionServicePtr{std::make_shared<EncryptionServiceV2>(m_dummyKeyProviderPtr)}
//      , m_propertyMetadataStoragePtr
//          {
//            [&propertyMetadataStorageFilename] -> std::shared_ptr<PropertyMetadataStorage>
//          {
//            auto propertyMetadataStoragePtr = std::make_shared<PropertyMetadataStorage>();
//            *propertyMetadataStoragePtr = PropertyMetadataStorage::from_file(propertyMetadataStorageFilename);
//            return propertyMetadataStoragePtr;
//          }()
//          }
//      , m_collectionServicePtr{ std::make_shared<CollectionService>(m_databasePtr, std::string(collectionName), m_encryptionServicePtr, m_propertyMetadataStoragePtr)}
//    {
//      m_clientPtr = std::make_shared<mongocxx::client>(mongocxx::uri{uri.data()});
//      m_databasePtr = std::make_shared<mongocxx::database>(m_clientPtr->database(databaseName.data()));
//      m_dummyKeyProviderPtr = std::make_shared<DummyKeyProvider>();
//      m_encryptionServicePtr = std::make_shared<EncryptionServiceV2>(m_dummyKeyProviderPtr);
//      m_propertyMetadataStoragePtr = [&propertyMetadataStorageFilename] -> std::shared_ptr<PropertyMetadataStorage>
//      {
//        auto propertyMetadataStoragePtr = std::make_shared<PropertyMetadataStorage>();
//        *propertyMetadataStoragePtr = PropertyMetadataStorage::from_file(propertyMetadataStorageFilename);
//        return propertyMetadataStoragePtr;
//      }();
//      m_collectionServicePtr = std::make_shared<CollectionService>(m_databasePtr, std::string(collectionName), m_encryptionServicePtr, m_propertyMetadataStoragePtr);
////      m_collectionServicePtr = std::shared_ptr<CollectionService>(
////        new CollectionService(
////          m_databasePtr, std::string(collectionName), m_encryptionServicePtr, m_propertyMetadataStoragePtr
////                             ));
//    };
//
//    auto collectionServicePtr() -> std::shared_ptr<CollectionService>
//    {
//      return m_collectionServicePtr;
//    }
//
//    auto databasePtr() -> std::shared_ptr<mongocxx::database>
//    {
//      return m_databasePtr;
//    }
//    private:
//    std::shared_ptr<mongocxx::client> m_clientPtr;
//    std::shared_ptr<mongocxx::database> m_databasePtr;
//    std::shared_ptr<EncryptionServiceV2> m_encryptionServicePtr;
//    std::shared_ptr<CollectionService> m_collectionServicePtr;
//    std::shared_ptr<PropertyMetadataStorage> m_propertyMetadataStoragePtr;
//    std::shared_ptr<DummyKeyProvider> m_dummyKeyProviderPtr;
//};