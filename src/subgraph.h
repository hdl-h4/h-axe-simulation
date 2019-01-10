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

#include "resource.h"
#include "shard_task.h"

namespace axe {
namespace simulation {

class SubGraph {
public:
  SubGraph() = default;

  inline auto &GetShardTasks() const { return shard_tasks_; }
  inline auto GetResourcePack() const { return resource_pack_; }
  inline auto GetWorkerId() const { return worker_id_; }
  inline void SetWorkerId(int worker_id) { worker_id_ = worker_id; }
  inline auto &GetDataLocality() const { return data_locality_; }
  inline auto GetMemory() const { return memory_; }

  friend void from_json(const json &j, SubGraph &sg) {
    j.at("shardtask").get_to(sg.shard_tasks_);
    sg.SetResourcesReq();
    sg.memory_ = sg.GetMemoryCap();
  }

  int GetMemoryCap() { return 0; }

  void SetResourcesReq() {
    for (auto &st : shard_tasks_) {
      if (st.GetResource() == ResourceType::cpu) {
        resource_pack_.cpu += st.GetReq();
      } else if (st.GetResource() == ResourceType::disk) {
        resource_pack_.disk += st.GetReq();
      } else if (st.GetResource() == ResourceType::network) {
        resource_pack_.network += st.GetReq();
      }
    }
    resource_pack_.memory = GetMemoryCap();
  }

  void Print() {
    std::cout << "worker id : " << worker_id_ << '\n';
    for (auto &st : shard_tasks_) {
      st.Print();
    }
  }

private:
  int memory_;
  ResourcePack resource_pack_;
  std::vector<ShardTask> shard_tasks_;
  std::vector<int> data_locality_;
  int worker_id_ = -1;
};

} // namespace simulation
} // namespace axe
