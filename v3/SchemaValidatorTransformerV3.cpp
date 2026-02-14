
#include "SchemaValidatorTransformerV3.h"

std::string SchemaValidatorTransformerV3::resolve_encryption_profile(std::string_view enc)
{
  // Legacy aliases from earlier PoC.
  if (enc == "AES") return "AES256_CTR";
  if (enc == "OPE") return "OPE_32_64";
  
  // Explicit supported profiles (CTR-only).
  if (enc == "AES256_CTR") return "AES256_CTR";
  if (enc == "AES192_CTR") return "AES192_CTR";
  if (enc == "OPE_32_64")  return "OPE_32_64";
  
  // Intentionally no GCM support.
  throw std::runtime_error(std::string("Unsupported encryption profile (V3 CTR-only): ") + std::string(enc));
}

std::tuple<ordered_json, PropertyMetadataStorage>
SchemaValidatorTransformerV3::transform_schema_validator(const ordered_json& document)
{
  ordered_json transformed_schema_validator{};
  
  require(document, "validator");
  require(document["validator"], "$jsonSchema");
  
  transformed_schema_validator["validator"]["$jsonSchema"] =
    transform_object_constraints(document["validator"]["$jsonSchema"]);
  
  save_paths(); // keep same behavior as current PoC
  return {transformed_schema_validator, std::move(this->property_metadata)};
}

ordered_json
SchemaValidatorTransformerV3::transform_object_constraints(const ordered_json& constraints)
{
  ordered_json result_metadata{};
  
  for (const auto& [key, value] : constraints.items())
  {
    if (key == "properties")
    {
      ordered_json transformed_object_properties{};
      
      for (const auto& [property_name, metadata] : value.items())
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
      // passthrough
      result_metadata[key] = value;
    }
  }
  
  return result_metadata;
}

ordered_json
SchemaValidatorTransformerV3::transform_array_constraints(const ordered_json& metadata)
{
  ordered_json result_metadata{};
  
  for (const auto& [key, value] : metadata.items())
  {
    if (key == "bsonType" && (value != "array"))
      throw std::runtime_error("SchemaValidatorTransformerV3: array_constraints bsonType != 'array'");
    
    if (key == "items")
    {
      auto& items_constraints = value;
      ordered_json transformed_items_constraints{};
      
      constexpr auto anonymous_property = "[0]";
      push_trunk(anonymous_property);
      
      auto& bsonType = items_constraints["bsonType"];
      if (bsonType == "object")
        transformed_items_constraints = transform_object_constraints(items_constraints);
      else if (bsonType == "array")
        transformed_items_constraints = transform_array_constraints(items_constraints);
      else
        transformed_items_constraints = transform_property_constraints(items_constraints);
      
      pop_trunk();
      
      result_metadata["items"] = transformed_items_constraints;
    }
    else
    {
      // passthrough
      result_metadata[key] = value;
    }
  }
  
  return result_metadata;
}

ordered_json
SchemaValidatorTransformerV3::transform_property_constraints(const ordered_json& constraints)
{
  ordered_json result{};
  
  bool use_bin_data = false;
  bool use_long_for_ope = false;
  
  ordered_json type_metadata = nullptr;
  bool save_type_metadata = false;
  
  for (const auto& [key, value] : constraints.items())
  {
    if (key == "bsonType" && (value == "object" || value == "array"))
      throw std::runtime_error("SchemaValidatorTransformerV3: property_constraints got object/array bsonType");
    
    if (key == "bsonType")
    {
      type_metadata = value;
      result[key] = value;
      continue;
    }
    
    if (key == "encryption")
    {
      // Normalize legacy / validate supported profiles (CTR-only).
      const auto profile = resolve_encryption_profile(value.get<std::string>());
      
      save_type_metadata = true;
      save_leaf();
      property_metadata.encryptions.emplace_back(profile);
      
      if (profile == "OPE_32_64")
        use_long_for_ope = true;
      else
        use_bin_data = true;
      
      // Do NOT copy `encryption` into schema output
      continue;
    }
    
    // passthrough
    result[key] = value;
  }
  
  if (use_bin_data && use_long_for_ope)
    throw std::runtime_error("SchemaValidatorTransformerV3: field cannot be encrypted with both AES and OPE");
  
  // OPE requires original bsonType exactly "int"; output becomes "long".
  if (use_long_for_ope)
  {
    if (type_metadata.is_null() || type_metadata != "int")
      throw std::runtime_error("SchemaValidatorTransformerV3: OPE encryption requires bsonType to be 'int'");
    result["bsonType"] = "long";
  }
  
  if (save_type_metadata)
    property_metadata.types.emplace_back(type_metadata);
  
  if (use_bin_data)
    result["bsonType"] = "binData";
  
  return result;
}

void SchemaValidatorTransformerV3::save_paths()
{
  // Same output format and paths as in your current version.
  {
    ordered_json result;
    std::ofstream output_file("../data/validator/types.json");
    for (const auto& type : property_metadata.types)
      result.emplace_back(type);
    output_file << std::setw(2) << result;
  }
  {
    ordered_json result{};
    std::ofstream output_file("../data/validator/paths.json");
    for (const auto& path : property_metadata.m_paths)
    {
      ordered_json path_array;
      for (const auto& path_node : path)
        path_array.emplace_back(path_node);
      result.emplace_back(path_array);
    }
    output_file << std::setw(2) << result;
  }
  {
    ordered_json result{};
    std::ofstream output_file("../data/validator/property_meta.json");
    for (int i = 0; i < static_cast<int>(paths().size()); ++i)
    {
      ordered_json property_spec;
      ordered_json path_array;
      for (const auto& path_node : paths().at(i))
        path_array.emplace_back(path_node);
      
      property_spec["property_path"] = path_array;
      property_spec["bsonType"] = property_metadata.types.at(i);
      property_spec["encryption"] = property_metadata.encryptions.at(i);
      result.emplace_back(property_spec);
    }
    output_file << std::setw(2) << result;
  }
}