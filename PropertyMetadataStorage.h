//
// Created by Bruno Kuś on 08/01/2026.
//

#ifndef MONGO_2_PROPERTYMETADATASTORAGE_H
#define MONGO_2_PROPERTYMETADATASTORAGE_H

#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

struct PropertyMetadataStorage
{
    std::vector<std::vector<std::string>> m_paths;
    std::vector<std::string> types;
    
    std::vector<std::string> encryptions;
    
    auto &paths()
    {
      return m_paths;
    }
    const auto& paths() const {return m_paths;}
    
    static PropertyMetadataStorage from_file(std::string_view filename)
    {
      
      std::ifstream input_file(filename.data());
      nlohmann::ordered_json document;
      input_file >> document;
      
      
      PropertyMetadataStorage result;
      for (const auto &property_meta: document)
      {
        std::vector<std::string> path;
        for (const auto &path_node: property_meta["property_path"])
        {
          path.emplace_back(path_node);
        }
        result.m_paths.emplace_back(path);
        auto type = property_meta["bsonType"];
        result.types.emplace_back(type);
        auto encryption = property_meta["encryption"];
        result.encryptions.emplace_back(encryption);
      }
      std::cout << "[from_file] encryptions.size()" << result.encryptions.size() << '\n';
      std::cout << "[from_file] types.size()" << result.types.size() << '\n';
      return result;
    }
    
    [[nodiscard]] nlohmann::ordered_json to_json() const
    {
      nlohmann::ordered_json result{};
//      std::ofstream output_file("../data/validator/property_meta.json");
      for (int i = 0; i < paths().size(); ++i)
      {
        nlohmann::ordered_json property_spec;
        nlohmann::ordered_json path_array;
        for (const auto &path_node: paths().at(i)) path_array.emplace_back(path_node);
        property_spec["property_path"] = path_array;
        property_spec["bsonType"] = types.at(i);
        property_spec["encryption"] = encryptions.at(i);
        result.emplace_back(property_spec);
      }
//      output_file << std::setw(2) << result;
      return result;
    }
};


#endif //MONGO_2_PROPERTYMETADATASTORAGE_H
