//
// Created by Bruno Kuś on 09/02/2026.
//
#include "CollectionService.h"

auto CollectionService::findManyFromCommand(
  const nlohmann::ordered_json &cmd,
  const mongocxx::options::find &options
)
//-> std::ranges::input_range auto
-> InputRangeOf<bsoncxx::document::value> auto
{
  if (cmd.contains("find") && cmd.at("find").is_string())
  {
    m_collectionName = cmd.at("find").get<std::string>();
  }
  
  const auto filter = cmd.contains("filter") ? cmd.at("filter") : nlohmann::ordered_json::object();
  return findManyFromJson(filter, options);
}

auto
CollectionService::findManyFromCommand(bsoncxx::document::view_or_value cmd, const mongocxx::options::find &options)
//-> std::ranges::input_range auto
-> InputRangeOf<bsoncxx::document::value> auto
{
  const auto json = ordered_json::parse(
    bsoncxx::to_json(cmd.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                       );
  return findManyFromCommand(json, options);
}

