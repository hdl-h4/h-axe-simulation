// Copyright 2018 H-AXE
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <fstream>
#include <string>

#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include "algorithm/algorithm_book.h"
#include "algorithm/task_placement.h"

namespace axe {
namespace simulation {

using json = nlohmann::json;

json ReadJsonFromFile(const std::string &file) {
  std::ifstream in(file, std::ios::in);
  CHECK(in.is_open()) << "Cannot open json file " << file;
  std::string ret;
  in.seekg(0, std::ios::end);
  ret.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&ret[0], ret.size());
  in.close();
  return json::parse(ret);
}

void PrintAlgorithm() {
  DLOG(INFO) << "Algorithm:";
  DLOG(INFO) << "TaskPlacement: " << AlgorithmBook::GetTaskPlacementString();
}

void SetAlgorithm(const json &j) {
  DLOG(INFO) << "set alg";
  std::string task_placement_str;
  j.at("TaskPlacement").get_to(task_placement_str);
  AlgorithmBook::Init(task_placement_str);
  PrintAlgorithm();
}
}
}
