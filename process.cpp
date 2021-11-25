//
// Created by X-ray on 11/23/2021.
//
#include "process.hpp"

#define BASE_URL "https://opendata.cbs.nl"

std::string GetBase(const std::string& url) {
  cpr::Response r = cpr::Get(cpr::Url{BASE_URL+url}, cpr::Header{{"Accept", "application/json"}});
  if (r.status_code == 200) {
    return r.text;
  } else {
    return {};
  }
}

std::string GetMeta(const std::string& url) {
  cpr::Response r = cpr::Get(cpr::Url{BASE_URL+url+"/$metadata"});
  if (r.status_code == 200) {
    return r.text;
  } else {
    return {};
  }
}