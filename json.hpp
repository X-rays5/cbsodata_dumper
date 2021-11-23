//
// Created by X-ray on 11/23/2021.
//

#pragma once

#ifndef CBSODATA_DUMPER_JSON_HPP
#define CBSODATA_DUMPER_JSON_HPP
namespace json {
  typedef rapidjson::GenericObject<false, rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>> rapidjson_object_t;
  typedef rapidjson::GenericObject<true, rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>> const_rapidjson_object_t;

  inline std::string GetString(rapidjson_object_t json, const char* key) {
   if (json.HasMember(key)) {
    if (json[key].IsString()) {
      return json[key].GetString();
    }
   }
   return {};
  }

  inline uint32_t GetInt(rapidjson_object_t json, const char* key) {
    if (json.HasMember(key)) {
      if (json[key].IsInt()) {
        return json[key].GetInt();
      }
    }
    return NULL;
  }

  inline uint64_t GetInt64(rapidjson_object_t json, const char* key) {
    if (json.HasMember(key)) {
      if (json[key].IsInt64()) {
        return json[key].GetInt64();
      }
    }
    return NULL;
  }

  inline bool GetBool(rapidjson_object_t json, const char* key) {
    if (json.HasMember(key)) {
      if (json[key].IsBool()) {
        return json[key].GetBool();
      }
    }
    return false;
  }

  inline rapidjson_object_t GetObject(rapidjson_object_t json, const char* key) {
    if (json.HasMember(key)) {
      if (json[key].IsObject()) {
        return json[key].GetObject();
      }
    }
    return json;
  }
}
#endif //CBSODATA_DUMPER_JSON_HPP
