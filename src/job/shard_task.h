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

#include "glog/logging.h"
#include "nlohmann/json.hpp"
#include "resource/resource.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace axe {
namespace simulation {

using nlohmann::json;

struct ShardTaskId {
  int task_id_;
  int shard_id_;
  friend void from_json(const json &j, ShardTaskId &id) {
    j.at("childtaskid").get_to(id.task_id_);
    j.at("childshardid").get_to(id.shard_id_);
  }
  bool operator<(const ShardTaskId &x) const {
    if (task_id_ != x.task_id_)
      return task_id_ < x.task_id_;
    return shard_id_ < x.shard_id_;
  }
};

class ShardTask {
public:
  ShardTask() {}

  inline auto GetTaskId() const { return task_id_; }
  inline auto GetShardId() const { return shard_id_; }
  inline auto &GetChildren() const { return children_; }
  inline auto GetResourceType() const { return resource_type_; }
  inline auto GetReq() const { return req_; }
  inline auto GetDuration() const { return duration_; }
  inline auto GetMemory() const { return memory_; }

  friend void from_json(const json &j, ShardTask &task) {
    j.at("taskid").get_to(task.task_id_);
    j.at("shardid").get_to(task.shard_id_);
    j.at("resource").get_to(task.resource_type_);
    j.at("request").get_to(task.req_);
    j.at("memory").get_to(task.memory_);
    j.at("duration").get_to(task.duration_);
    auto pos = j.find("children");
    if (pos != j.end()) {
      pos->get_to(task.children_);
    }
  }

  void Print() {
    DLOG(INFO) << "task_id : " << task_id_;
    DLOG(INFO) << "shard_id : " << shard_id_;
    DLOG(INFO) << "resource : " << resource_type_;
    DLOG(INFO) << "request : " << req_;
    DLOG(INFO) << "duration : " << duration_;
    DLOG(INFO) << "memory : " << memory_;
    DLOG(INFO) << "children : ";
    for (auto &child : children_) {
      DLOG(INFO) << "{" << child.task_id_ << ", " << child.shard_id_ << "}, ";
    }
  }

private:
  int task_id_ = -1;
  int shard_id_;
  int resource_type_;
  double memory_;
  double req_;
  double duration_;
  std::vector<ShardTaskId> children_;
};

} // namespace simulation
} // namespace axe
