#include <nlohmann/json_fwd.hpp>
#include <bsoncxx/builder/basic/document-fwd.hpp>
#include <bsoncxx/builder/basic/array-fwd.hpp>
#include <bsoncxx/document/view.hpp>


auto json_to_bson_array(const nlohmann::ordered_json &json) -> bsoncxx::builder::basic::array;
auto json_to_bson_document(const nlohmann::ordered_json &json) -> bsoncxx::builder::basic::document;
auto
append_json_kvp(bsoncxx::builder::basic::document &out, const std::string &key, const nlohmann::ordered_json &v) -> void;

using namespace nlohmann;

ordered_json fromFile(std::string_view filename);

ordered_json jsonFromFile(std::string_view filename);


void toFile(std::string_view filename, const ordered_json &document);

void jsonToFile(std::string_view filename, const ordered_json &document);

void debug_document(bsoncxx::document::view view);