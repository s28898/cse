//
// Created by Bruno Kuś on 05/11/2025.
//

#ifndef MONGO_2_SCHEMAVALIDATORTRANSFORMER_V2_H
#define MONGO_2_SCHEMAVALIDATORTRANSFORMER_V2_H

#include <vector>
#include <string>
#include <string_view>
#include <tuple>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "../PropertyMetadataStorage.h"

using nlohmann::ordered_json;

struct SchemaValidatorTransformerV2
{
    public:
    std::tuple<ordered_json, PropertyMetadataStorage> transform_schema_validator(const ordered_json& document);

    ordered_json transform_object_constraints(const ordered_json& metadata);
    ordered_json transform_array_constraints(const ordered_json& metadata);
    ordered_json transform_property_constraints(const ordered_json& metadata);

    PropertyMetadataStorage property_metadata;
    std::vector<std::string> trunk;

    private:
    static void require(const ordered_json& document, std::string_view key)
    {
      if (!document.contains(key))
        throw std::runtime_error(std::string("SchemaValidatorTransformerV2: missing key: ") + std::string(key));
    }

    static std::string resolve_encryption_profile(std::string_view enc);

    std::vector<std::vector<std::string>>& paths() { return property_metadata.m_paths; }

    void save_leaf()
    {
      std::vector<std::string> saved_path{};
      saved_path.insert(saved_path.begin(), trunk.begin(), trunk.end());
      paths().emplace_back(std::move(saved_path));
    }

    void push_trunk(std::string trunk_node)
    {
      std::cout << "pushing: " << trunk_node << '\n';
      trunk.emplace_back(std::move(trunk_node));
    }

    void pop_trunk()
    {
      if (trunk.empty())
        throw std::runtime_error("SchemaValidatorTransformerV2: pop_trunk on empty stack");
      trunk.pop_back();
    }

    void save_paths();
};

#endif // MONGO_2_SCHEMAVALIDATORTRANSFORMER_V2_H
