//
// Created by X-ray on 11/25/2021.
//

#pragma once

#ifndef CBSODATA_DUMPER_UTIL_HPP
#define CBSODATA_DUMPER_UTIL_HPP
namespace cbsodata {
    namespace util {
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
    }
}
#endif //CBSODATA_DUMPER_UTIL_HPP
