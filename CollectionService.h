//
// Created by Bruno Kuś on 09/02/2026.
//

#ifndef MONGO_2_COLLECTIONSERVICE_H
#define MONGO_2_COLLECTIONSERVICE_H

#include <ranges>

#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

#include <bsoncxx/json.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/stdx/optional.hpp>

#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/cursor.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/options/insert.hpp>
#include <mongocxx/options/update.hpp>
#include <mongocxx/options/delete.hpp>
#include <mongocxx/result/insert_one.hpp>
#include <mongocxx/result/insert_many.hpp>
#include <mongocxx/result/update.hpp>
#include <mongocxx/result/delete.hpp>

#include "v2/StatementTransformerV2.h"
#include "v2/BsonDecryptorV2.h"
#include "v2/EncryptionService_v2.h"
#include "PropertyMetadataStorage.h"
#include "CursorView.h"
#include "MongoDriverSubset.h"

#include "./concepts.h"

class CollectionService
{
    public:
    CollectionService(
      std::shared_ptr<mongocxx::database> db,
      std::string collectionName,
      std::shared_ptr<EncryptionServiceV2> encryptionService,
      std::shared_ptr<PropertyMetadataStorage> storage
    )
      : m_db(std::move(db)), m_collectionName(std::move(collectionName)),
        m_encryptionService(std::move(encryptionService)), m_storage(std::move(storage))
    {
      if (!m_db) throw std::runtime_error("CollectionService: db == null");
      if (!m_encryptionService) throw std::runtime_error("CollectionService: encryptionService == null");
      if (!m_storage) throw std::runtime_error("CollectionService: storage == null");
    }
    
    // function with deduced return type
    auto findManyFromJson(
      const nlohmann::ordered_json &filter,
      const mongocxx::options::find &options = {}
    )
    -> InputRangeOf<bsoncxx::document::value> auto
    
    {
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      
      const bsoncxx::document::value transformedFilter = st.transform_statement_deep(filter);
      
      auto cursor = (*m_db)[m_collectionName].find(transformedFilter.view(), options);
      
      return
        CursorView(std::move(cursor))
        | std::views::transform(
          [decryptor = BsonDecryptorV2{m_collectionName, m_encryptionService, m_storage}]
            (const auto &doc) mutable -> bsoncxx::document::value
          {
            return decryptor.decrypt_deep(doc);
          }
                               );
    }
    // ============================================================
    // 1) camelCase “main” API
    // ============================================================
    
    auto findMany(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::find &options = {}
    )
    -> InputRangeOf<bsoncxx::document::value> auto
    {
      const auto json = ordered_json::parse(
        bsoncxx::to_json(filter.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                           );
      return findManyFromJson(json, options);
    }
    
    auto findOne(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::find &options = {}
    )
    -> bsoncxx::stdx::optional<bsoncxx::document::value>
    {
      const auto json = ordered_json::parse(
        bsoncxx::to_json(filter.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                           );
      return findOneFromJson(json, options);
    }
    
    auto insertOne(
      bsoncxx::document::view_or_value document,
      const mongocxx::options::insert &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::insert_one>
    {
      const auto json = ordered_json::parse(
        bsoncxx::to_json(document.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                           );
      return insertOneFromJson(json, options);
    }
    
    template<class Container>
    auto insertMany(
      const Container &documents,
      const mongocxx::options::insert &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::insert_many>
    {
      ordered_json arr = ordered_json::array();
      for (const auto &d: documents)
      {
        arr.push_back(
          ordered_json::parse(
            bsoncxx::to_json(d.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                             ));
      }
      return insertManyFromJson(arr, options);
    }
    
    auto updateOne(
      bsoncxx::document::view_or_value filter,
      bsoncxx::document::view_or_value update,
      const mongocxx::options::update &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::update>
    {
      const auto q = ordered_json::parse(
        bsoncxx::to_json(filter.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                        );
      const auto u = ordered_json::parse(
        bsoncxx::to_json(update.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                        );
      return updateOneFromJson(q, u, options);
    }
    
    auto updateMany(
      bsoncxx::document::view_or_value filter,
      bsoncxx::document::view_or_value update,
      const mongocxx::options::update &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::update>
    {
      const auto q = ordered_json::parse(
        bsoncxx::to_json(filter.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                        );
      const auto u = ordered_json::parse(
        bsoncxx::to_json(update.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                        );
      return updateManyFromJson(q, u, options);
    }
    
    auto deleteOne(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::delete_options &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::delete_result>
    {
      const auto q = ordered_json::parse(
        bsoncxx::to_json(filter.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                        );
      return deleteOneFromJson(q, options);
    }
    
    auto deleteMany(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::delete_options &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::delete_result>
    {
      const auto q = ordered_json::parse(
        bsoncxx::to_json(filter.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                        );
      return deleteManyFromJson(q, options);
    }
    
    // ============================================================
    // 2) driver-like lowercase overloads (mongocxx naming)
    // ============================================================
    
    auto find(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::find &options = {}
    )
    -> std::ranges::input_range auto { return findMany(filter, options); }
    
    auto find_one(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::find &options = {}
    )
    -> bsoncxx::stdx::optional<bsoncxx::document::value> { return findOne(filter, options); }
    
    auto insert_one(
      bsoncxx::document::view_or_value document,
      const mongocxx::options::insert &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::insert_one> { return insertOne(document, options); }
    
    template<class Container>
    auto insert_many(
      const Container &documents,
      const mongocxx::options::insert &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::insert_many> { return insertMany(documents, options); }
    
    auto update_one(
      bsoncxx::document::view_or_value filter,
      bsoncxx::document::view_or_value update,
      const mongocxx::options::update &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::update> { return updateOne(filter, update, options); }
    
    auto update_many(
      bsoncxx::document::view_or_value filter,
      bsoncxx::document::view_or_value update,
      const mongocxx::options::update &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::update> { return updateMany(filter, update, options); }
    
    auto delete_one(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::delete_options &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::delete_result> { return deleteOne(filter, options); }
    
    auto delete_many(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::delete_options &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::delete_result> { return deleteMany(filter, options); }
    
    // ============================================================
    // 3) snake_case overloads
    // ============================================================
    
    auto find_many(
      bsoncxx::document::view_or_value filter,
      const mongocxx::options::find &options = {}
    )
    -> InputRangeOf<bsoncxx::document::value> auto
    { return find(filter, options); }
    
    // ============================================================
    // 4) Extra helpers
    // ============================================================
    
   
    
    auto findOneFromJson(
      const nlohmann::ordered_json &filter,
      const mongocxx::options::find &options = {}
    )
    -> bsoncxx::stdx::optional<bsoncxx::document::value>
    {
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      const bsoncxx::document::value transformedFilter = st.transform_statement_deep(filter);
      
      auto res = (*m_db)[m_collectionName].find_one(transformedFilter.view(), options);
      if (!res) return {};
      
      BsonDecryptorV2 decryptor{m_collectionName, m_encryptionService, m_storage};
      return decryptor.decrypt_deep(res->view());
    }
    
    auto insertOneFromJson(
      const nlohmann::ordered_json &document,
      const mongocxx::options::insert &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::insert_one>
    {
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      const bsoncxx::document::value transformed = st.transform_statement_deep(document);
      return m_db->collection(m_collectionName).insert_one(transformed.view(), options);
    }
    
    auto insertManyFromJson(
      const nlohmann::ordered_json &documents,
      const mongocxx::options::insert &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::insert_many>
    {
      if (!documents.is_array())
      {
        throw std::runtime_error("CollectionService::insertManyFromJson: expected JSON array");
      }
      
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      
      std::vector<bsoncxx::document::value> transformedDocs;
      transformedDocs.reserve(documents.size());
      for (const auto &doc: documents)
        transformedDocs.push_back(st.transform_statement_deep(doc));
      
      return (*m_db)[m_collectionName].insert_many(transformedDocs, options);
    }
    
    auto updateOneFromJson(
      const nlohmann::ordered_json &filter,
      const nlohmann::ordered_json &update,
      const mongocxx::options::update &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::update>
    {
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      const auto q = st.transform_statement_deep(filter);
      const auto u = st.transform_statement_deep(update);
      return (*m_db)[m_collectionName].update_one(q.view(), u.view(), options);
    }
    
    auto updateManyFromJson(
      const nlohmann::ordered_json &filter,
      const nlohmann::ordered_json &update,
      const mongocxx::options::update &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::update>
    {
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      const auto q = st.transform_statement_deep(filter);
      const auto u = st.transform_statement_deep(update);
      return (*m_db)[m_collectionName].update_many(q.view(), u.view(), options);
    }
    
    auto deleteOneFromJson(
      const nlohmann::ordered_json &filter,
      const mongocxx::options::delete_options &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::delete_result>
    {
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      const auto q = st.transform_statement_deep(filter);
      return (*m_db)[m_collectionName].delete_one(q.view(), options);
    }
    
    auto deleteManyFromJson(
      const nlohmann::ordered_json &filter,
      const mongocxx::options::delete_options &options = {}
    )
    -> bsoncxx::stdx::optional<mongocxx::result::delete_result>
    {
      StatementTransformerV2 st{
        m_collectionName,
        m_encryptionService,
        m_storage};
      const auto q = st.transform_statement_deep(filter);
      return (*m_db)[m_collectionName].delete_many(q.view(), options);
    }
    
    auto findManyFromCommand(
      const nlohmann::ordered_json &cmd,
      const mongocxx::options::find &options = {}
    )
//    -> std::ranges::input_range auto;
    -> InputRangeOf<bsoncxx::document::value> auto;

    
    auto findManyFromCommand(
      bsoncxx::document::view_or_value cmd,
      const mongocxx::options::find &options = {}
    )
//    -> std::ranges::input_range auto;
    -> InputRangeOf<bsoncxx::document::value> auto;

    
    private:
    std::shared_ptr<mongocxx::database> m_db;
    std::string m_collectionName;
    std::shared_ptr<EncryptionServiceV2> m_encryptionService;
    std::shared_ptr<PropertyMetadataStorage> m_storage;
};

static_assert(MongoDriverSimpleSubset<mongocxx::collection>);
static_assert(MongoDriverSimpleSubset<CollectionService>);


#endif //MONGO_2_COLLECTIONSERVICE_H
