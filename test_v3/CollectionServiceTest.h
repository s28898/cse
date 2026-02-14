//
// Created by Bruno Kuś on 12/02/2026.
//

#ifndef MONGO_2_COLLECTIONSERVICETEST_H
#define MONGO_2_COLLECTIONSERVICETEST_H

#include <nlohmann/json.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include <bsoncxx/json.hpp>

#include "../DummyKeyProvider.h"
#include "../v3/EncryptionServiceV3.h"
#include "../v3/StatementTransformerV3.h"
#include "../v3/SchemaValidatorTransformerV3.h"
#include "../v3/CollectionServiceV3.h"

using namespace std::literals::string_view_literals;




inline
auto overrideCollection(
  mongocxx::database &db,
  std::string_view collectionName,
  const nlohmann::ordered_json &validatorTransformed
) -> void
{
  if (db.has_collection(collectionName.data()))
  {
    db.collection(collectionName.data()).drop();
  }
  db.create_collection(collectionName, bsoncxx::from_json(validatorTransformed.dump()));
}

inline nlohmann::json strip_oid(nlohmann::json arr)
{
  for (auto &element: arr) element.erase("_id");
  return arr;
}

inline nlohmann::ordered_json jsonArrayFromCursor(auto &cursor)
{
  auto result = nlohmann::ordered_json::array();
  for (const auto &doc: cursor)
  {
    result.emplace_back(
      nlohmann::json::parse(bsoncxx::to_json(doc.view(), bsoncxx::ExtendedJsonMode::k_relaxed))
                       );
  }
  return result;
}
inline
auto findAll(CollectionServiceV3 &collectionService) -> ordered_json
{
  InputRangeOf<bsoncxx::document::value> auto cursor  = collectionService.find_many(bsoncxx::builder::basic::make_document());
  return jsonArrayFromCursor(cursor);
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
    return SchemaValidatorTransformerV3{}.transform_schema_validator(schemaValidatorExt);
  }();
  jsonToFile(schemaValidatorTransformedOutputFilename, validatorTransformed);
  jsonToFile(propertyMetadataStorageOutputFilename, propertyMetadata.to_json());
}

struct DependencyContainer
{
    
    DependencyContainer(
      std::string_view propertyMetadataStorageFilename,
      std::string_view keyPolicyFilename,
      std::string_view uri,
      std::string_view databaseName,
      std::string_view collectionName
    )
    {
      m_clientPtr = std::make_shared<mongocxx::client>(mongocxx::uri{uri.data()});
      m_databasePtr = std::make_shared<mongocxx::database>((*m_clientPtr)[databaseName.data()]);
      m_databaseKeyPolicyPtr = [&keyPolicyFilename] -> std::shared_ptr<DatabaseKeyPolicy>
      {
        auto policyPtr = std::make_shared<DatabaseKeyPolicy>();
        *policyPtr = DatabaseKeyPolicy::fromFile(keyPolicyFilename);
        return policyPtr;
      }();
      m_dummyKeyProviderPtr = std::make_shared<DummyKeyProvider>(m_databaseKeyPolicyPtr, "../dummy/context");
      m_encryptionServicePtr = std::make_shared<EncryptionServiceV3>(m_dummyKeyProviderPtr);
      m_propertyMetadataStoragePtr = [&propertyMetadataStorageFilename] -> std::shared_ptr<PropertyMetadataStorage>
      {
        auto propertyMetadataStoragePtr = std::make_shared<PropertyMetadataStorage>();
        *propertyMetadataStoragePtr = PropertyMetadataStorage::from_file(propertyMetadataStorageFilename);
        assert(not propertyMetadataStoragePtr->to_json().empty());
        std::cerr << propertyMetadataStoragePtr->to_json() << '\n';
        return propertyMetadataStoragePtr;
      }();
      m_collectionServicePtr = std::make_shared<CollectionServiceV3>(
        m_databasePtr, std::string(collectionName), m_encryptionServicePtr, m_propertyMetadataStoragePtr
                                                                    );
      assert(m_collectionServicePtr != nullptr);
    };
    
    auto collectionServicePtr() const -> std::shared_ptr<CollectionServiceV3>
    {
      return m_collectionServicePtr;
    }
    
    auto databasePtr() const -> std::shared_ptr<mongocxx::database>
    {
      return m_databasePtr;
    }
    
    auto keyPolicyPtr() const -> std::shared_ptr<DatabaseKeyPolicy> { return m_databaseKeyPolicyPtr; };
    
    auto encryptionServicePtr() const -> auto
    {
      return m_encryptionServicePtr;
    }
    
    private:
    std::shared_ptr<mongocxx::client> m_clientPtr;
    std::shared_ptr<mongocxx::database> m_databasePtr;
    std::shared_ptr<DatabaseKeyPolicy> m_databaseKeyPolicyPtr;
    std::shared_ptr<DummyKeyProvider> m_dummyKeyProviderPtr;
    std::shared_ptr<EncryptionServiceV3> m_encryptionServicePtr;
    std::shared_ptr<PropertyMetadataStorage> m_propertyMetadataStoragePtr;
    std::shared_ptr<CollectionServiceV3> m_collectionServicePtr;
};

#endif //MONGO_2_COLLECTIONSERVICETEST_H
