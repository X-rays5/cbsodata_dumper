//
// Created by X-ray on 11/23/2021.
//

#pragma once

#ifndef CBSODATA_DUMPER_PROCESS_HPP
#define CBSODATA_DUMPER_PROCESS_HPP
#include <string>
#include <cpr/cpr.h>

std::string GetBase(const std::string& url);
std::string GetMeta(const std::string& url);
#endif //CBSODATA_DUMPER_PROCESS_HPP
