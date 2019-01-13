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

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace axe {
namespace simulation {

using nlohmann::json;

enum ResourceType { cpu = 1, memory, network, disk };

class ShardTask {
public:
  ShardTask() {}

  inline auto GetTaskId() const { return task_id_; }
  inline auto GetShardId() const { return shard_id_; }
  inline auto &GetChildren() const { return children_; }
  inline auto GetResource() const { return resource_; }
  inline auto GetReq() const { return req_; }
  inline auto GetDuration() const { return duration_; }
  inline auto GetMemory() const { return memory_; }

  friend void from_json(const json &j, ShardTask &task) {
    j.at("taskid").get_to(task.task_id_);
    j.at("shardid").get_to(task.shard_id_);
    j.at("resource").get_to(task.resource_);
    j.at("request").get_to(task.req_);
    j.at("memory").get_to(task.memory_);
    j.at("duration").get_to(task.duration_);
    auto pos = j.find("children");
    if (pos != j.end()) {
      pos->get_to(task.children_);
    }
  }

  void Print() {
    std::cout << "task_id : " << task_id_ << '\n';
    std::cout << "shard_id : " << shard_id_ << '\n';
    std::cout << "resource : " << resource_ << '\n';
    std::cout << "request : " << req_ << '\n';
    std::cout << "duration : " << duration_ << '\n';
    std::cout << "memory : " << memory_ << '\n';
    std::cout << "children : ";
    for (auto &child : children_) {
      std::cout << "{" << child.first << ", " << child.second << "}, ";
    }
    std::cout << '\n';
  }

private:
  int task_id_ = -1;
  int shard_id_;
  ResourceType resource_;
  int memory_;
  int req_;
  int duration_;
  std::vector<std::pair<int, int>> children_;
};

} // namespace simulation
} // namespace axe
