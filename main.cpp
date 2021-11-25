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
#include "util.hpp"

#define FAIL(reason) failed_mtx.lock();failed[dataset_name]=reason;failed_mtx.unlock();return;
#define OPEN_FILE(filename) writer.open("data/"+dataset_name+"/"+filename)

void DumpAll(const std::string& url, const std::string& dataset_name, std::unordered_map<std::string, std::string>& failed, std::mutex& failed_mtx) {
  std::string base = GetBase(url);
  if (!base.empty()) {
    cbsodata::util::CreateDirIfNotExist("data/"+dataset_name);

    std::ofstream writer{};

    OPEN_FILE("base.json");
    writer << base;
    writer.close();

    OPEN_FILE("metadata.xml");
    writer << GetMeta(url);
    writer.close();

    rapidjson::Document json;
    if (!json.Parse(base.c_str()).HasParseError()) {
      if (json.HasMember("value") && json["value"].IsArray()) {
        for (rapidjson::SizeType i = 0; i< json["value"].Size(); i++) {
          if (json["value"][i].IsObject()) {
            auto val = json["value"][i].GetObject();
            cbsodata::util::CreateDirIfNotExist("data/"+dataset_name+"/"+json::GetString(val, "name"));

            cpr::Response r = cpr::Get(cpr::Url{json::GetString(val, "url")}, cpr::Header{{"Accept", "application/json"}});
            if (r.status_code == 200) {
              OPEN_FILE(json::GetString(val, "name")+"/data.json");
              writer << r.text;
              writer.close();

              rapidjson::Document data;
              if (!data.Parse(r.text.c_str()).HasParseError()) {
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

int main(int argc, char* argv[]) {
  std::uint32_t thread_count = std::thread::hardware_concurrency();
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <thread count>\n";
    std::cerr << "Max concurrent threads supported: " << thread_count << std::endl;
    return EXIT_FAILURE;
  } else {
    try {
      thread_count = std::stoi(argv[1]);
      if (thread_count <= 0) {
        std::cerr << "Thread count must be greater than 0\n";
        return EXIT_FAILURE;
      }
    } catch (std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  }

  std::cout << "Getting all datasets" << std::endl;
  cpr::Response r = cpr::Get(cpr::Url{"https://opendata.cbs.nl/odatafeed"});

  if (r.status_code != 200) {
    std::cout << "Error: " << r.status_code << std::endl;
    return EXIT_FAILURE;
  } else {
    std::unordered_map<std::string, std::string> failed;
    std::mutex failed_mtx;
    auto start_time = std::chrono::high_resolution_clock::now();
    {
      std::cout << "Parsing list" << std::endl;
      std::vector<std::string> lines = cbsodata::util::split(r.text);
      job_queue q(thread_count);

      cbsodata::util::CreateDirIfNotExist("data");

      std::regex regex("<a.*href=(.*)>.*<\\/a>");
      std::smatch smatch;
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
        std::cout << "Downloads queued: " << q.count() << " Fails: " << failed.size() << '\n';
        std::cout << "Time passed: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count() << "s" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }
    for (auto&& fail : failed) {
      std::cout << "Failed to get " << fail.first << ": " << fail.second << '\n';
    }
    std::cout << "Finished in: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count() << "s Fails: " << failed.size() << std::endl;
  }
  return EXIT_SUCCESS;
}