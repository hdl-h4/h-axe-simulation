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

#include "algorithm/task_placement.h"
#include "resource/resource_request.h"
#include "worker/worker.h"
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace axe {
namespace simulation {

class AlgorithmBook {
public:
  static void Init() {
    task_placement = "FIFO";
    task_placement_book = {{"FIFO", FIFO}, {"TETRIS", TETRIS}};
  }
  static void Init(std::string str) {
    task_placement = str;
    task_placement_book = {{"FIFO", FIFO}, {"TETRIS", TETRIS}};
  }
  static auto GetTaskPlacement() { return task_placement_book[task_placement]; }
  static std::string GetTaskPlacementString() { return task_placement; }
  static void SetTaskPlacement(std::string str) { task_placement = str; }

private:
  static std::string task_placement;
  static std::map<std::string,
                  std::function<std::vector<std::pair<int, ResourceRequest>>(
                      std::multimap<double, ResourceRequest> &,
                      std::vector<std::shared_ptr<WorkerAbstract>> &)>>
      task_placement_book;
};

} // namespace simulation
} // namespace axe
