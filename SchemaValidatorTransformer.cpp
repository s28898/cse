//
// Created by Bruno Kuś on 05/11/2025.
//

#include "SchemaValidatorTransformer.h"

std::tuple<ordered_json, PropertyMetadataStorage>
SchemaValidatorTransformer::transform_schema_validator(const ordered_json &document)
{
  ordered_json transformed_schema_validator{};
  
  require(document, "validator");
  require(document["validator"], "$jsonSchema");
  transformed_schema_validator["validator"]["$jsonSchema"] =
    transform_object_constraints(document["validator"]["$jsonSchema"]);
  
  save_paths(); // ? TODO
  return {transformed_schema_validator, std::move(this->property_metadata)};
}


ordered_json
SchemaValidatorTransformer::transform_object_constraints(const ordered_json &constraints)
{
  ordered_json result_metadata{};
  for (const auto &[key, value]: constraints.items())
  {
//        std::cout << "transform_object_property_validator :: key=" << key << "\n";
//            if (key == "bsonType" && value != "object") throw std::runtime_error("");
    /*  if (key == "required")
      {
        result_metadata[key] = value;
      }
      else */
    if (key == "properties")
    {
      ordered_json transformed_object_properties{};
      for (const auto &[property_name, metadata]: value.items())
      {
        if (metadata["bsonType"] == "object")
        {
          push_trunk(property_name);
          transformed_object_properties[property_name] = transform_object_constraints(metadata);
          pop_trunk();
        }
        else if (metadata["bsonType"] == "array")
        {
          push_trunk(property_name);
          transformed_object_properties[property_name] = transform_array_constraints(metadata);
          pop_trunk();
        }
        else
        {
          push_trunk(property_name);
          transformed_object_properties[property_name] = transform_property_constraints(metadata);
          pop_trunk();
        }
      }
      result_metadata["properties"] = transformed_object_properties;
    }
    else
    {
      result_metadata[key] = value;
      continue;
    }
  }
  return result_metadata;
}


ordered_json
SchemaValidatorTransformer::transform_array_constraints(const ordered_json &metadata)
{
  std::cout << "transform_array_constraints" << '\n';
  /*
   * array property, opisuje jakie są wymagania dla itemsów, okej
   * i to jest normalnie pośród propertisów
   */
  ordered_json result_metadata{};
  for (const auto &[key, value]: metadata.items())
  {
    if (key == "bsonType" && (value != "array")) throw std::runtime_error("");
    
    // items jest jak nazwa atrybutu wewnątrz bloku poperties
    if (key == "items")
    {
      auto &items_constraints = value;
      ordered_json transformed_items_constraints{};
      
      constexpr auto anonymous_property = "[0]";
      push_trunk(anonymous_property);
      auto &bsonType = items_constraints["bsonType"];
      if (bsonType == "object")
      {
        transformed_items_constraints = transform_object_constraints(items_constraints);
      }
      else if (bsonType == "array")
      {
        transformed_items_constraints = transform_array_constraints(items_constraints);
      }
      else
      {
        transformed_items_constraints = transform_property_constraints(items_constraints);
      }
      pop_trunk();
      result_metadata["items"] = transformed_items_constraints;
//      if (items_constraints["bsonType"] == "object") {
//        push_trunk("[0]");
//        transformed_items_constraints["[0]"] = transform_object_constraints(metadata);
//        pop_trunk();
//      } else if (items_constraints["bsonType"] == "array") {
//        push_trunk("[0]");
//        transformed_items_constraints
//        pop_trunk();
//      }
//      result_metadata["items"] = transform_object_constraints(value);
    
    }
    else
    {
      result_metadata[key] = value;
    }
  }
  return result_metadata;
}

ordered_json
SchemaValidatorTransformer::transform_property_constraints(const ordered_json &constraints)
{
  std::cerr << "[SchemaValidatorTransformer::transform_property_constraints] enter\n";
  ordered_json result{};
  bool use_bin_data = false;
  bool use_long_for_ope = false;
  ordered_json type_metadata = nullptr;
  bool save_type_metadata = false;
  
  for (const auto &[key, value]: constraints.items())
  {
    std::cerr << "key : " << key << ", value : " << value << '\n';
    if (key == "bsonType" && (value == "object" || value == "array")) { throw std::runtime_error(""); }
    else if (key == "bsonType")
    {
      std::cerr << "pushing type — " << value << '\n';
      type_metadata = value;
//      property_metadata.types.clear();
//      property_metadata.types.emplace_back(value);
      result[key] = value;
    }
    else if (key == "encryption" && value == "AES")
    {
      save_type_metadata = true;
      save_leaf();
      property_metadata.encryptions.emplace_back("AES");
      use_bin_data = true;
    }
    else if (key == "encryption" && value == "OPE")
    {
      save_type_metadata = true;
      save_leaf();
      property_metadata.encryptions.emplace_back("OPE");
      use_long_for_ope = true;
    }
    else
    {
      result[key] = value;
    }
  }
  if (use_bin_data && use_long_for_ope)
  {
    throw std::runtime_error("Field cannot be encrypted with both AES and OPE");
  }

  // OPE requires the original bsonType to be exactly "int"; after transformation it becomes "long".
  if (use_long_for_ope)
  {
    if (type_metadata.is_null() || type_metadata != "int")
    {
      throw std::runtime_error("OPE encryption requires bsonType to be 'int'");
    }
    result["bsonType"] = "long";
  }

  if (save_type_metadata) { property_metadata.types.emplace_back(type_metadata); }
  if (use_bin_data) result["bsonType"] = "binData";
  std::cerr << "[SchemaValidatorTransformer::transform_property_constraints] exit\n";

  return result;
}
