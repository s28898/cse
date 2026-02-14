//
// Created by Bruno Kuś on 08/01/2026.
//

#ifndef MONGO_2_ABSTRACTTEST_H
#define MONGO_2_ABSTRACTTEST_H

#include "SchemaValidatorTransformer.h"
#include "StatementTransformer.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/result/insert_many.hpp>
#include <mongocxx/result/update.hpp>
#include <mongocxx/result/delete.hpp>

#include <ranges>
#include <iterator>
#include <sstream>
#include <utility>
#include <optional>
#include <limits>
#include "BsonDecryptor.h"
#include "CommandTransformer.h"

#include <iterator>

#define NON_CONSTEXPR auto

struct OpeSimpleTest
{
    OpeSimpleTest() = default;
    
    
    static auto makeTest(std::string collection_name, std::string_view path) -> OpeSimpleTest
    {
      std::stringstream ss;
      ss << path << '/' << collection_name << '/' << (collection_name + "_validator.json");
      auto input_file_schema_validator_ext = ss.str();
      ss = {};
      ss << path << '/' << collection_name << '/' << "output" << '/'
         << (collection_name + "_validator_transformed.json");
      auto output_file_transformed_schema_validator = ss.str();
      ss = {};
      ss << path << '/' << collection_name << '/' << "output" << '/' << (collection_name + "_property_meta.json");
      auto output_file_property_meta = ss.str();
      collection_name = collection_name;
      
      EncryptionService encryption_agent_1;
      encryption_agent_1.load("../secret/context.bin");
      std::string_view uri{"mongodb://localhost:27017"};
      std::string_view dbname{"testdb"};
      return {
        input_file_schema_validator_ext, output_file_transformed_schema_validator, output_file_property_meta,
        collection_name,
        std::move(encryption_agent_1), uri, dbname
      };
    }
    
    OpeSimpleTest(
      std::string_view input_file_schema_validator_ext,
      std::string_view output_file_transformed_schema_validator,
      std::string_view output_file_property_meta,
      std::string_view collection_name,
      EncryptionService encryption_agent,
      std::string_view uri,
      std::string_view dbname
    )
      : m_input_file_schema_validator_ext{input_file_schema_validator_ext},
        m_output_file_transformed_schema_validator{output_file_transformed_schema_validator},
        m_output_file_property_meta{output_file_property_meta},
        m_collection_name{collection_name},
        m_encryption_agent{std::move(encryption_agent)},
        
        m_client{mongocxx::uri{uri.data()}},
        m_db{m_client[dbname.data()]}
    {
    
    }
    
    ~OpeSimpleTest()
    {
    
    }
    
    
    std::string m_input_file_schema_validator_ext;
    std::string m_output_file_transformed_schema_validator;
    std::string m_output_file_property_meta;
    
    std::string m_collection_name;
    EncryptionService m_encryption_agent;
    PropertyMetadataStorage m_storage; // read only
    
    
    mongocxx::client m_client; //{mongocxx::uri{"mongodb://localhost:27017"}};
    mongocxx::database m_db; // = client["testdb"];
    
    void createCollection()
    {
      auto schema_validator_ext = fromFile(m_input_file_schema_validator_ext);
      if (schema_validator_ext.is_null())
      {
        throw std::runtime_error(
          std::string("fromFile returned null for schema validator: ") + m_input_file_schema_validator_ext
                                );
      }
      if (!schema_validator_ext.is_object())
      {
        throw std::runtime_error(
          std::string("schema validator must be an object. file: ") + m_input_file_schema_validator_ext +
          ", got: " + schema_validator_ext.dump());
      }
      SchemaValidatorTransformer transformer{};
      auto [final_schema, propertyMetadataStorage] =
        transformer.transform_schema_validator(schema_validator_ext);
      m_storage = propertyMetadataStorage;
      toFile(m_output_file_transformed_schema_validator, final_schema);
      toFile(m_output_file_property_meta, propertyMetadataStorage.to_json());
      if (m_db.has_collection(m_collection_name)) { m_db[m_collection_name].drop(); }
      m_db.create_collection(m_collection_name, bsoncxx::from_json(final_schema.dump()));
    }
    
    static auto resolve_collection_name(const nlohmann::ordered_json &cmd, const std::string &fallback) -> std::string
    {
      // Supports placeholders like "__COLLECTION__" or "COLLECTION" used in test fixtures.
      auto resolve = [&](const nlohmann::ordered_json &v) -> std::string
      {
        if (!v.is_string()) return fallback;
        const auto s = v.get<std::string>();
        if (s == "__COLLECTION__" || s == "COLLECTION") return fallback;
        return s;
      };
      
      for (const char *key: {"find", "insert", "update", "delete"})
      {
        if (cmd.contains(key))
        {
          return resolve(cmd.at(key));
        }
      }
      return fallback;
    }
    
    struct PlainCursor
    {
        std::vector<bsoncxx::document::value> docs;
        
        struct It
        {
            const std::vector<bsoncxx::document::value> *v{};
            std::size_t i{};
            
            auto operator*() const -> bsoncxx::document::view { return (*v)[i].view(); }
            
            auto operator++() -> It &
            {
              ++i;
              return *this;
            }
            
            auto operator!=(const It &other) const -> bool { return i != other.i; }
        };
        
        auto begin() const -> It { return It{&docs, 0}; }
        
        auto end() const -> It { return It{&docs, docs.size()}; }
        
        auto size() const -> std::size_t { return docs.size(); }
        
        auto empty() const -> bool { return docs.empty(); }
    };
    
    

//    static_assert(std::ranges::view<CursorView>);
//    static_assert(std::ranges::viewable_range<CursorView>);
    
    
    
    auto runCommandRawFromStatement(const nlohmann::ordered_json &statement) -> bsoncxx::document::value
    {
      CommandTransformer command_transformer{m_encryption_agent, m_storage};
      bsoncxx::document::value command_transformed = command_transformer.transform_command(statement);
      std::cerr << "command_transformed\n" << std::setw(2) << ordered_json::parse(
        bsoncxx::to_json(command_transformed.view(), bsoncxx::ExtendedJsonMode::k_relaxed)) << '\n';
      return m_db.run_command(std::move(command_transformed));
    }
    
    // Convenience for tests: if the command is a find, decrypt cursor.firstBatch and return {"result": [...]}
    auto runCommandFromStatement(const nlohmann::ordered_json &statement) -> bsoncxx::document::value
    {
      const bool is_find = statement.contains("find");
      auto raw = runCommandRawFromStatement(statement);
      if (!is_find) return raw;
      
      // Normalize find reply into the same shape as document_from_cursor: {"result": [...]}
      const auto json = ordered_json::parse(bsoncxx::to_json(raw.view(), bsoncxx::ExtendedJsonMode::k_relaxed));
      if (!json.contains("cursor") || !json["cursor"].contains("firstBatch"))
      {
        return raw;
      }
      
      BsonDecryptor decryptor{m_encryption_agent, m_storage};
      bsoncxx::builder::basic::array result_array{};
      
      for (const auto &el: json["cursor"]["firstBatch"])
      {
        // Convert each batch document back to BSON, decrypt, and append.
        auto as_bson = bsoncxx::from_json(el.dump());
        result_array.append(decryptor.decrypt_deep(as_bson.view()));
      }
      
      bsoncxx::builder::basic::document out{};
      out.append(bsoncxx::builder::basic::kvp("result", result_array.extract()));
      return out.extract();
    }
    
    auto
    insertMany(const nlohmann::ordered_json &documents) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::insert_many>
    {
//      m_db[m_collection_name].inse
      StatementTransformer st = makeStatementTransformer();
      auto transformed_docs =
        documents
        | std::views::transform(
          [&](const auto &doc) -> bsoncxx::v_noabi::document::value { return st.transform_statement_deep(doc); }
                               )
        | std::ranges::to<std::vector>();
      return m_db[m_collection_name].insert_many(transformed_docs);
    }
    
   
    
    auto findMany(const nlohmann::ordered_json &filter) -> PlainCursor
    {
      StatementTransformer st = makeStatementTransformer();
      const bsoncxx::document::value transformed_filter = st.transform_statement_deep(filter);
      auto cursor = m_db[m_collection_name].find(transformed_filter.view());
      
      PlainCursor out;
      BsonDecryptor decryptor{m_encryption_agent, m_storage};
      for (const auto &doc: cursor)
        out.docs.push_back(decryptor.decrypt_deep(doc));
      return out;
    }
    
    auto updateOne(
      const nlohmann::ordered_json &filter, const nlohmann::ordered_json &update,
      const mongocxx::options::update &options = {}
    ) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::update>
    {
      StatementTransformer st = makeStatementTransformer();
      const bsoncxx::document::value q = st.transform_statement_deep(filter);
      const bsoncxx::document::value u = st.transform_statement_deep(update);
      return m_db[m_collection_name].update_one(q.view(), u.view(), options);
    }
    
    auto updateMany(
      const nlohmann::ordered_json &filter, const nlohmann::ordered_json &update,
      const mongocxx::options::update &options = {}
    ) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::update>
    {
      StatementTransformer st = makeStatementTransformer();
      const bsoncxx::document::value q = st.transform_statement_deep(filter);
      const bsoncxx::document::value u = st.transform_statement_deep(update);
      return m_db[m_collection_name].update_many(q.view(), u.view(), options);
    }
    
    auto
    deleteOne(const nlohmann::ordered_json &filter) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::delete_result>
    {
      StatementTransformer st = makeStatementTransformer();
      const bsoncxx::document::value q = st.transform_statement_deep(filter);
      return m_db[m_collection_name].delete_one(q.view());
    }
    
    auto
    deleteMany(const nlohmann::ordered_json &filter) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::delete_result>
    {
      StatementTransformer st = makeStatementTransformer();
      const bsoncxx::document::value q = st.transform_statement_deep(filter);
      return m_db[m_collection_name].delete_many(q.view());
    }
    
    // ---- Command-shaped overloads (runCommand-compatible fixtures) ----
    
    auto
    insertManyFromCommand(const nlohmann::ordered_json &cmd) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::insert_many>
    {
      // {"insert": <coll>, "documents": [...]}
      if (!cmd.contains("documents")) throw std::runtime_error("insert command missing 'documents'");
      if (cmd.at("documents").is_null() || !cmd.at("documents").is_array())
      {
        throw std::runtime_error(
          std::string("insert command expects 'documents' array. documents=") + cmd.at("documents").dump());
      }
      m_collection_name = resolve_collection_name(cmd, m_collection_name);
      return insertMany(cmd.at("documents"));
    }
    
    auto findManyFromCommand(const nlohmann::ordered_json &cmd) -> PlainCursor
    {
      // {"find": <coll>, "filter": {...}}
      if (cmd.contains("filter") && (cmd.at("filter").is_null() || !cmd.at("filter").is_object()))
      {
        throw std::runtime_error(
          std::string("find command expects 'filter' object. filter=") + cmd.at("filter").dump());
      }
      m_collection_name = resolve_collection_name(cmd, m_collection_name);
      const auto filter = cmd.contains("filter") ? cmd.at("filter") : nlohmann::ordered_json::object();
      return findMany(filter);
    }
    
    auto
    updateFromCommand(const nlohmann::ordered_json &cmd) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::update>
    {
      // {"update": <coll>, "updates": [{"q": {...}, "u": {...}, "multi": bool, "upsert": bool}]}
      if (!cmd.contains("updates") || !cmd.at("updates").is_array() || cmd.at("updates").empty())
      {
        throw std::runtime_error("update command missing 'updates'");
      }
      
      m_collection_name = resolve_collection_name(cmd, m_collection_name);
      const auto &upd0 = cmd.at("updates").at(0);
      
      if (!upd0.contains("q") || !upd0.contains("u"))
      {
        throw std::runtime_error("update spec missing 'q' or 'u'");
      }
      
      const auto &q = upd0.at("q");
      const auto &u = upd0.at("u");
      
      if (q.is_null() || u.is_null())
      {
        throw std::runtime_error(std::string("update spec has null q/u. q=") + q.dump() + ", u=" + u.dump());
      }
      
      if (!q.is_object())
      {
        throw std::runtime_error(std::string("update spec expects 'q' to be an object. q=") + q.dump());
      }
      
      if (!u.is_object())
      {
        throw std::runtime_error(std::string("update spec expects 'u' to be an object. u=") + u.dump());
      }
      
      const bool multi = upd0.contains("multi") ? upd0.at("multi").get<bool>() : false;
      
      mongocxx::options::update opts;
      if (upd0.contains("upsert")) opts.upsert(upd0.at("upsert").get<bool>());
      
      if (multi)
      {
        return updateMany(q, u, opts);
      }
      return updateOne(q, u, opts);
    }
    
    auto
    deleteFromCommand(const nlohmann::ordered_json &cmd) -> bsoncxx::v_noabi::stdx::optional<mongocxx::result::delete_result>
    {
      // {"delete": <coll>, "deletes": [{"q": {...}, "limit": 0|1}]}
      if (!cmd.contains("deletes") || !cmd.at("deletes").is_array() || cmd.at("deletes").empty())
      {
        throw std::runtime_error("delete command missing 'deletes'");
      }
      
      m_collection_name = resolve_collection_name(cmd, m_collection_name);
      const auto &del0 = cmd.at("deletes").at(0);
      if (!del0.contains("q")) throw std::runtime_error("delete spec missing 'q'");
      const int limit = del0.contains("limit") ? del0.at("limit").get<int>() : 0;
      
      const auto &q = del0.at("q");
      if (q.is_null())
      {
        throw std::runtime_error(std::string("delete spec has null q. q=") + q.dump());
      }
      if (!q.is_object())
      {
        throw std::runtime_error(std::string("delete spec expects 'q' to be an object. q=") + q.dump());
      }
      
      if (limit == 1)
      {
        return deleteOne(q);
      }
      return deleteMany(q);
    }
    
    auto insertManyFromStatement(const nlohmann::json &statement)
    {
      StatementTransformer statement_transformer{m_encryption_agent, m_storage};
      
      const auto &docs_to_insert = statement;
      auto transformed_docs =
        docs_to_insert
        | std::views::transform(
          [&statement_transformer]
            (const auto &doc) { return statement_transformer.transform_statement_deep(doc); }
                               )
        | std::ranges::to<std::vector>();
      std::cout << "=== PRINTING TRANSFORMED DOCS TO INSERT ===" << "\n";
      for (const auto &doc: transformed_docs)
      {
        std::cout << std::setw(2)
                  << ordered_json::parse(bsoncxx::to_json(doc.view(), bsoncxx::ExtendedJsonMode::k_relaxed)) << "\n";
        debug_document(doc.view());
      }
      m_db[m_collection_name].insert_many(transformed_docs);
    }
    
    auto insertMany(const std::string_view filename) const -> void
    {
//      auto property_meta = PropertyMetadataStorage::from_file(m_output_file_property_meta);
      StatementTransformer statement_transformer{m_encryption_agent, m_storage};
//      statement_transformer.m_paths = property_meta.paths();
      auto docs_to_insert = fromFile(filename);
      if (docs_to_insert.is_null())
      {
        throw std::runtime_error(std::string("fromFile returned null for insert fixture: ") + std::string(filename));
      }
      if (!docs_to_insert.is_array())
      {
        throw std::runtime_error(
          std::string("insert fixture must be an array. file: ") + std::string(filename) +
          ", got: " + docs_to_insert.dump());
      }
      std::cout << "=== RAW DOCKS TO INSERT ===\n";
      std::cout << std::setw(2) << docs_to_insert << "\n";
      
      
      auto transformed_docs =
        docs_to_insert
        | std::views::transform(
          [&statement_transformer]
            (const auto &doc) { return statement_transformer.transform_statement_deep(doc); }
                               )
        | std::ranges::to<std::vector>();
      std::cout << "=== PRINTING TRANSFORMED DOCS TO INSERT ===" << "\n";
      for (const auto &doc: transformed_docs)
      {
        std::cout << std::setw(2)
                  << ordered_json::parse(bsoncxx::to_json(doc.view(), bsoncxx::ExtendedJsonMode::k_relaxed)) << "\n";
        debug_document(doc.view());
      }
      m_db[m_collection_name].insert_many(transformed_docs);
    }
    
    
    // to nie powinno przyjmować nazwy pliku, tylko powinienem mieć dwa osobne kroki
    // i najpierw mam wczytywanie
    
    
    // returns plain json
    
    static auto json_from_bson(bsoncxx::document::value bson) -> nlohmann::json
    {
      const auto json_string = bsoncxx::to_json(bson.view(), bsoncxx::ExtendedJsonMode::k_relaxed);
      const auto json = nlohmann::json::parse(json_string);
      return json;
    }
    
    auto makeStatementTransformer() -> StatementTransformer
    {
      return {m_encryption_agent, m_storage};
    }
    
    auto findManyFromStatement(const nlohmann::ordered_json &statement) -> std::string
    {
      if (statement.is_null())
      {
        throw std::runtime_error("findManyFromStatement: filter is null");
      }
      if (!statement.is_object())
      {
        throw std::runtime_error(std::string("findManyFromStatement: filter must be object, got: ") + statement.dump());
      }
      // Keep old test API: statement is treated as filter document.
      auto cursor = findMany(statement);
      
      bsoncxx::builder::basic::array result_array{};
      for (auto v: cursor) result_array.append(v);
      
      bsoncxx::builder::basic::document out{};
      out.append(bsoncxx::builder::basic::kvp("result", result_array.extract()));
      return bsoncxx::to_json(out.view(), bsoncxx::ExtendedJsonMode::k_relaxed);
    }
    
    auto findManyFromFile(const std::string_view filename) -> std::string
    {
      auto filter = fromFile(filename);
      if (filter.is_null())
      {
        throw std::runtime_error(std::string("fromFile returned null for find fixture: ") + std::string(filename));
      }
      if (!filter.is_object())
      {
        throw std::runtime_error(
          std::string("find fixture must be an object (filter document). file: ") + std::string(filename) +
          ", got: " + filter.dump());
      }
      return findManyFromStatement(filter);
    }
    
    auto findManyAndPrint(const std::string_view filename) -> void
    {
      auto statement = fromFile(filename);
//      auto property_meta = PropertyMetadataStorage::from_file(m_output_file_property_meta);
      StatementTransformer statement_transformer{m_encryption_agent, m_storage};
//      statement_transformer.m_paths = property_meta.paths();
      
      auto transformed_statement = statement_transformer.transform_statement_deep(statement);
      auto cursor = m_db[m_collection_name].find(transformed_statement.view());
      auto result_document = document_from_cursor(std::move(cursor));
//
      std::cout << "=== PRINTING QUERY RESULT ===" << "\n";
      std::cout << bsoncxx::to_json(result_document.view(), bsoncxx::ExtendedJsonMode::k_relaxed) << "\n";
    }
    
    auto document_from_cursor(mongocxx::v_noabi::cursor cursor) -> bsoncxx::document::value
    {
      using bsoncxx::builder::basic::kvp;
      
      bsoncxx::builder::basic::array result_array{};
//
      BsonDecryptor decryptor{m_encryption_agent, m_storage};
      for (const auto &document: cursor)
      {
        debug_document(document);
        result_array.append(decryptor.decrypt_deep(document));
//        result_array.append(decrypt_bson_document(encryption_agent, document));
      }
      
      bsoncxx::builder::basic::document result_document{};
      result_document.append(kvp("result", result_array.extract()));
      return result_document.extract();
    }
    
    static auto run() -> void
    {
      
      OpeSimpleTest test = makeTest("ope_simple", "../test/data");
      test.createCollection();
      test.insertMany("../test/data/ope_simple/insert1.json");
      test.findManyAndPrint("../test/data/ope_simple/find1.json");
    }
  
  
};

#endif //MONGO_2_ABSTRACTTEST_H
