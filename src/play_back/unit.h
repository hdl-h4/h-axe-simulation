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

#include <cstdint>
#include <string>

#include "glog/logging.h"
#include "nlohmann/json.hpp"

namespace axe {
namespace simulation {

using nlohmann::json;

class Unit {
public:
  inline int64_t GetAssignedTime() const { return assigned_time_; }
  inline int64_t GetFinishedTime() const { return finished_time_; }
  inline std::string GetLocality() const { return locality_; }
  inline int GetWorkerID() const { return worker_id_; }
  inline int GetResourceType() const { return resource_type_; }
  inline double GetMemory() const { return memory_; }
  inline double GetResourceRequest() const { return resource_req_; }
  inline int GetTaskID() const { return task_id_; }
  inline int GetShardID() const { return shard_id_; }
  inline int GetInstanceID() const { return instance_id_; }

  void SetAssignedTime(int64_t time) { assigned_time_ = time; }

  void SetFinishedTime(int64_t time) { finished_time_ = time; }

  friend void from_json(const json &j, Unit &unit) {
    j.at("assigned").get_to(unit.assigned_time_);
    j.at("finished").get_to(unit.finished_time_);
    j.at("locality").get_to(unit.locality_);
    j.at("resource_type").get_to(unit.resource_type_);
    j.at("memory").get_to(unit.memory_);
    j.at("resource_req").get_to(unit.resource_req_);
    j.at("task_id").get_to(unit.task_id_);
    j.at("shard_id").get_to(unit.shard_id_);
    j.at("instance_id").get_to(unit.instance_id_);
    unit.SetWorkerIDByLocality();
  }

  void SetWorkerIDByLocality() {
    locality_.erase(0, 6);
    //  std::cout << locality_ << std::endl;
    worker_id_ = std::stoi(locality_);
  }

private:
  int64_t assigned_time_;
  int64_t finished_time_;
  std::string locality_;
  int worker_id_;
  int resource_type_;
  double memory_;
  double resource_req_;
  int task_id_;
  int shard_id_;
  int instance_id_;
};
}
}
