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

#include "shard_task.h"

namespace axe {
namespace simulation {

class SubGraph {
public:
  SubGraph() = default;

  inline auto &GetShardTasks() const { return shard_tasks_; }
  inline auto &GetResoucesReq() const { return resources_req_; }
  inline auto &GetWorkerId() const { return worker_id_; }
  inline void SetWorkerId(int worker_id) { worker_id_ = worker_id; }

  friend void from_json(const json &j, SubGraph &sg) {
    j.at("shardtask").get_to(sg.shard_tasks_);
  }

  void SetResourcesReq() {
    for (auto &st : shard_tasks_) {
      resources_req_[st.GetResource()] += st.GetReq();
    }
  }

  void Print() {
    std::cout << "worker id : " << worker_id_ << '\n';
    for (auto &st : shard_tasks_) {
      st.Print();
    }
  }

private:
  std::map<Resource, int> resources_req_;
  std::vector<ShardTask> shard_tasks_;
  int worker_id_;
};

} // namespace simulation
} // namespace axe
