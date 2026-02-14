//
// Created by Bruno Kuś on 08/01/2026.
//

#ifndef MONGO_2_UTIL_H
#define MONGO_2_UTIL_H


#include <nlohmann/json.hpp>
#include <fstream>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include "EncryptionService_v1.h"

#include "bson_util.h"

template<class... Ts>
struct overloaded : Ts ...
{
    using Ts::operator()...;
};
int64_t bytes_to_i64(std::string_view plain);
bsoncxx::document::value decrypt_bson_document(EncryptionService &encryption_agent, bsoncxx::document::view document);

//auto json_to_bson_array(const nlohmann::ordered_json &json) -> bsoncxx::builder::basic::array;
//auto json_to_bson_document(const nlohmann::ordered_json &json) -> bsoncxx::builder::basic::document;
//auto
//append_json_kvp(bsoncxx::builder::basic::document &out, const std::string &key, const nlohmann::ordered_json &v) -> void;
//
//using namespace nlohmann;

//ordered_json fromFile(std::string_view filename);
//
//ordered_json jsonFromFile(std::string_view filename);
//
//void toFile(std::string_view filename, const ordered_json &document);
//
//void jsonToFile(std::string_view filename, const ordered_json &document);
//
//void debug_document(bsoncxx::document::view view);

#endif //MONGO_2_UTIL_H
