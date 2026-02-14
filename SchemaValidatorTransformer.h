//
// Created by Bruno Kuś on 05/11/2025.
//

#ifndef MONGO_1_SCHEMAVALIDATORTRANSFORMER_H
#define MONGO_1_SCHEMAVALIDATORTRANSFORMER_H



#include <vector>
#include <string>
#include <nlohmann/json.hpp>

using namespace nlohmann;

#include <fstream>

#include <iostream>


#include "PropertyMetadataStorage.h"

struct SchemaValidatorTransformer
{
    public:
    std::tuple<ordered_json, PropertyMetadataStorage> transform_schema_validator(const ordered_json &document);
    
    ordered_json transform_object_constraints(const ordered_json &metadata);
    
    ordered_json transform_array_constraints(const ordered_json &metadata);
    
    ordered_json transform_property_constraints(const ordered_json &metadata);
    
//    private:
    PropertyMetadataStorage property_metadata;
    std::vector<std::string> trunk;
    
    std::vector<std::vector<std::string>> &paths()
    {
      return property_metadata.m_paths;
    }
    
    void dump_paths()
    {
      auto dump_path = [](const auto &path)
      {
        if (!path.empty())
        {
          auto head = path.begin();
          std::cout << *head;
          if (path.size() > 1)
          {
            auto tail = path | std::views::drop(1);
            for (const auto &item: tail) std::cout << ", " << item;
          }
          std::cout << "\n";
        }
      };
      std::ranges::for_each(paths(), dump_path);
    }
    
    void save_leaf()
    {
//      std::cerr << "save_leaf(" << leaf << ")" << "\n";
      std::vector<std::string> saved_path{};
//      saved_path.reserve(trunk.size() + 1);
      saved_path.insert(saved_path.begin(), trunk.begin(), trunk.end());
//      saved_path.emplace_back(std::move(leaf));
      
      paths().emplace_back(std::move(saved_path));
    }
    
    void push_trunk(std::string trunk_node)
    {
      std::cout << "pushing: " << trunk_node << '\n';
      trunk.emplace_back(std::move(trunk_node));
    }
    
    void pop_trunk()
    {
      if (trunk.empty()) throw std::runtime_error("");
      trunk.pop_back();
    }
 
    
    
    private:
    inline static void require(const ordered_json &document, std::string_view key)
    {
      if (!document.contains(key)) throw std::runtime_error("");
    }
    
    void save_paths()
    {
      {
        ordered_json result;
        std::ofstream output_file("../data/validator/types.json");
        for (const auto &type: property_metadata.types)
        {
          result.emplace_back(type);
        }
        output_file << std::setw(2) << result;
      }
      {
        ordered_json result{};
        std::ofstream output_file("../data/validator/paths.json");
        for (const auto &path: property_metadata.m_paths)
        {
          ordered_json path_array;
          for (const auto &path_node: path)
          {
            path_array.emplace_back(path_node);
          }
          result.emplace_back(path_array);
        }
        output_file << std::setw(2) << result;
      }
      {
        ordered_json result{};
        std::ofstream output_file("../data/validator/property_meta.json");
        for (int i = 0; i < paths().size(); ++i)
        {
          ordered_json property_spec;
          ordered_json path_array;
          for (const auto &path_node: paths().at(i)) path_array.emplace_back(path_node);
          property_spec["property_path"] = path_array;
          property_spec["bsonType"] = property_metadata.types.at(i);
          property_spec["encryption"] = property_metadata.encryptions.at(i);
          result.emplace_back(property_spec);
        }
        output_file << std::setw(2) << result;
        
      }
    }
};




#endif //MONGO_1_SCHEMAVALIDATORTRANSFORMER_H
