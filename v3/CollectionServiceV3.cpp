// CollectionServiceV3.cpp
//
// Created by Bruno Kuś on 12/02/2026.
//

#include "CollectionServiceV3.h"

auto CollectionServiceV3::findManyFromCommand(
  const nlohmann::ordered_json &cmd,
  const mongocxx::options::find &options
)
-> InputRangeOf<bsoncxx::document::value> auto
{
  // Minimal “run_command-like” shape for find:
  // { "find": "<collection>", "filter": { ... }, ... }
  //
  // This is CollectionService, so we DO NOT allow overriding the collection name here.
  if (cmd.contains("find"))
  {
    if (!cmd.at("find").is_string())
      throw std::runtime_error("CollectionServiceV3::findManyFromCommand: 'find' must be a string");
    
    const auto requested = cmd.at("find").get<std::string>();
    if (requested != m_collectionName)
      throw std::runtime_error(
        "CollectionServiceV3::findManyFromCommand: command targets collection '" + requested +
        "', but this service is bound to '" + m_collectionName + "'"
                              );
  }
  
  const auto filter = cmd.contains("filter")
                      ? cmd.at("filter")
                      : nlohmann::ordered_json::object();
  
  return findManyFromJson(filter, options);
}

auto CollectionServiceV3::findManyFromCommand(
  bsoncxx::document::view_or_value cmd,
  const mongocxx::options::find &options
)
-> InputRangeOf<bsoncxx::document::value> auto
{
  const auto json = ordered_json::parse(
    bsoncxx::to_json(cmd.view(), bsoncxx::ExtendedJsonMode::k_relaxed)
                                       );
  return findManyFromCommand(json, options);
}