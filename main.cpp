#include <iostream>
#include <regex>
#include <chrono>
#include <unordered_map>
#include <atomic>
#include <filesystem>
#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include "job_queue.hpp"
#include "process.hpp"
#include "json.hpp"

#define FAIL(reason) failed_mtx.lock();failed[dataset_name]=reason;failed_mtx.unlock();return;
#define OPEN_FILE(filename) writer.open("data/"+dataset_name+"/"+filename)

// replace every occurrence of a string with another string
inline std::string replace_all(std::string src, const std::string& target, const std::string& replace_to) {
  size_t start_pos = 0;
  while((start_pos = src.find(target, start_pos)) != std::string::npos) {
    src.replace(start_pos, target.length(), replace_to);
    start_pos += replace_to.length();
  }
  return src;
}

// split string by newline
std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> ret;
    std::string::const_iterator i = s.begin();
    std::string::const_iterator j = s.begin();
    while (i != s.end()) {
        if (*i == '\n') {
            ret.emplace_back(std::string(j, i));
            ++i;
            j = i;
        } else {
            ++i;
        }
    }
    if (j != s.end()) {
        ret.emplace_back(std::string(j, s.end()));
    }
    return ret;
}

inline void CreateDirIfNotExist(const std::string& dir) {
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }
}

void DumpAll(const std::string& url, const std::string& dataset_name, std::unordered_map<std::string, std::string>& failed, std::mutex& failed_mtx) {
  std::string base = GetBase(url);
  if (!base.empty()) {
    CreateDirIfNotExist("data/"+dataset_name);

    std::ofstream writer{};

    OPEN_FILE("base.json");
    writer << base;
    writer.close();

    OPEN_FILE("metadata.xml");
    writer << GetMeta(url);
    writer.close();

    std::string default_filter = "?";
    rapidjson::Document json;
    if (!json.Parse(base.c_str()).HasParseError()) {
      if (json.HasMember("value") && json["value"].IsArray()) {
        for (rapidjson::SizeType i = 0; i< json["value"].Size(); i++) {
          if (json["value"][i].IsObject()) {
            auto val = json["value"][i].GetObject();
            CreateDirIfNotExist("data/"+dataset_name+"/"+json::GetString(val, "name"));
            retry:
            cpr::Response r = cpr::Get(cpr::Url{json::GetString(val, "url")});
            if (r.status_code == 200) {
              OPEN_FILE(json::GetString(val, "name")+"/data.json");
              writer << r.text;
              writer.close();

              rapidjson::Document data;
              if (!data.Parse(r.text.c_str()).HasParseError()) {
                if (json::GetString(val, "name") == "TableInfos") {
                  if (data.HasMember("value") && data["value"].IsArray()) {
                    default_filter = replace_all(data["value"][0]["DefaultSelection"].GetString(), " ", "%20");
                  }
                }
                if (data.HasMember("odata.metadata")) {
                  r = cpr::Get(cpr::Url(data["odata.metadata"].GetString()));
                  if (r.status_code == 200) {
                    OPEN_FILE(json::GetString(val, "name")+"/metadata.xml");
                    writer << r.text;
                    writer.close();
                  } else {
                    FAIL(r.text)
                  }
                }
              } else {
                FAIL("Error parsing data")
              }
            } else {
              if (r.text.find("less than 10000 records") != std::string::npos) {
                if (json::GetString(val, "url").rfind(default_filter) == std::string::npos) {
                  std::string tmp = json::GetString(val, "url");
                  tmp.append("?");
                  tmp.append(default_filter);
                  val.RemoveMember("url");
                  val.AddMember("url", rapidjson::Value(tmp.c_str(), json.GetAllocator()), json.GetAllocator());
                  goto retry;
                }
              }
              FAIL(r.text)
            }
          } else {
            FAIL("Entry in base meta data value array is not a object")
          }
        }
      } else {
        FAIL("Missing value array in base meta data")
      }
    } else {
      FAIL("Couldn't parse base meta data")
    }
  } else {
    FAIL("Base meta data unavailable")
  }
}

int main() {
  std::cout << "Getting all datasets" << std::endl;
  cpr::Response r = cpr::Get(cpr::Url{"https://opendata.cbs.nl/odataapi"});

  if (r.status_code != 200) {
    std::cout << "Error: " << r.status_code << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "Parsing list" << std::endl;
    std::vector<std::string> lines = split(r.text);
    job_queue q(50);

    CreateDirIfNotExist("data");

    std::unordered_map<std::string, std::string> failed;
    std::mutex failed_mtx;

    std::regex regex("<a.*href=(.*)>.*<\\/a>");
    std::smatch smatch;
    auto start_time = std::chrono::high_resolution_clock::now();
    for (auto&& line : lines) {
      if (std::regex_search(line, smatch, regex)) {
        if (smatch.size() > 1) {
          q.add_job([=, &failed, &failed_mtx]{
            DumpAll(smatch[1], smatch[1].str().substr(smatch[1].str().rfind('/') + 1), failed, failed_mtx);
          });
        }
      }
    }

    std::cout << "Jobs queued waiting to finish\n";
    while(q.count() > 0) {
      std::cout << "Jobs queued: " << q.count() << " Fails: " << failed.size() << '\n';
      std::cout << "Time passed: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count() << "s" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    for (auto&& fail : failed) {
      std::cout << "Failed to get " << fail.first << ": " << fail.second << '\n';
    }
    std::cout << "Finished in: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count() << "s Fails: " << failed.size() << std::endl;
  }
  return EXIT_SUCCESS;
}