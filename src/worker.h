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

#include <vector>

#include "job/shard_task.h"
#include "resource/resource.h"

#include "nlohmann/json.hpp"

#include <iostream>

namespace axe {
namespace simulation {

using nlohmann::json;

class Worker {
public:
  Worker(){};
  Worker(double cpu, double memory, double disk, double network)
      : cpu_(cpu), memory_(memory), disk_(disk), net_(network) {}

  friend void from_json(const json &j, Worker &worker) {
    j.at("cpu").get_to(worker.cpu_);
    j.at("memory").get_to(worker.memory_);
    j.at("disk").get_to(worker.disk_);
    j.at("network").get_to(worker.net_);
  }

  inline auto GetRemainResourcePack() const {
    return ResourcePack(cpu_ - cpu_counter_, memory_ - memory_counter_,
                        disk_ - disk_counter_, net_ - net_counter_);
  }

  // task finish
  std::vector<ShardTask> TaskFinish(const ShardTask &task) {
    std::vector<ShardTask> tasks;
    if (task.GetResource() == ResourceType::cpu) {
      cpu_counter_ -= task.GetReq();
      while (cpu_queue_.size() != 0 &&
             cpu_queue_.at(0).GetReq() + cpu_counter_ < cpu_) {
        tasks.push_back(cpu_queue_.at(0));
        cpu_counter_ += cpu_queue_.at(0).GetReq();
        cpu_queue_.erase(cpu_queue_.begin());
      }
    } else if (task.GetResource() == ResourceType::network) {
      net_counter_ -= task.GetReq();
      while (net_queue_.size() != 0 &&
             net_queue_.at(0).GetReq() + net_counter_ < net_) {
        tasks.push_back(net_queue_.at(0));
        net_counter_ += net_queue_.at(0).GetReq();
        net_queue_.erase(net_queue_.begin());
      }
    } else {
      disk_counter_ -= task.GetReq();
      while (disk_queue_.size() != 0 &&
             disk_queue_.at(0).GetReq() + disk_counter_ < disk_) {
        tasks.push_back(disk_queue_.at(0));
        disk_counter_ += disk_queue_.at(0).GetReq();
        disk_queue_.erase(disk_queue_.begin());
      }
    }
    return tasks;
  }

  // subgraph finish
  void SubGraphFinish(double mem) { memory_counter_ -= mem; }

  // place new task, return true : task run; false : task waits in queue;
  bool PlaceNewTask(const ShardTask &task) {
    if (task.GetResource() == ResourceType::cpu) {
      if (cpu_queue_.size() == 0 && cpu_counter_ + task.GetReq() < cpu_) {
        cpu_counter_ += task.GetReq();
        return true;
      } else {
        cpu_queue_.push_back(task);
        return false;
      }
    } else if (task.GetResource() == ResourceType::network) {
      if (net_queue_.size() == 0 && net_counter_ + task.GetReq() < net_) {
        net_counter_ += task.GetReq();
        return true;
      } else {
        net_queue_.push_back(task);
        return false;
      }
    } else {
      if (disk_queue_.size() == 0 && disk_counter_ + task.GetReq() < disk_) {
        disk_counter_ += task.GetReq();
        return true;
      } else {
        disk_queue_.push_back(task);
        return false;
      }
    }
  }

  void Print() {
    DLOG(INFO) << "cpu: " << cpu_;
    DLOG(INFO) << "mem: " << memory_;
    DLOG(INFO) << "disk: " << disk_;
    DLOG(INFO) << "net: " << net_;
  }

private:
  double cpu_;
  double memory_;
  double disk_;
  double net_;
  double cpu_counter_ = 0;
  double memory_counter_ = 0;
  double disk_counter_ = 0;
  double net_counter_ = 0;
  std::vector<ShardTask> cpu_queue_;
  std::vector<ShardTask> disk_queue_;
  std::vector<ShardTask> net_queue_;
};

} //  namespace simulation
} //  namespace axe
