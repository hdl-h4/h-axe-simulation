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

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <set>
#include <vector>

#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include "event/task_finish_event.h"
#include "job/shard_task.h"
#include "resource/resource.h"
#include "worker_common.h"

namespace axe {
namespace simulation {

struct DiskNetworkRunningTaskRecord {
  int event_id;
  ShardTask shard_task;
  double last_check_time;
  double last_check_data_size;
  double last_check_bandwidth;
};

class WorkerCommon {
public:
  WorkerCommon() {}
  WorkerCommon(int worker_id, std::shared_ptr<ResourcePack> resource_capacity,
               std::shared_ptr<ResourcePack> resource_usage,
               std::shared_ptr<ResourcePack> resource_reservation,
               bool is_worker)
      : worker_id_(worker_id), resource_capacity_(resource_capacity),
        resource_usage_(resource_usage),
        resource_reservation_(resource_reservation), is_worker_(is_worker) {}

  virtual std::vector<std::shared_ptr<Event>>
  PlaceNewTask(double time, const ShardTask &task) = 0;

  virtual std::vector<std::shared_ptr<Event>>
  TaskFinish(double time, int event_id, const ShardTask &task) = 0;

protected:
  virtual double ComputeNewTaskDuration(const ShardTask &task) = 0;
  virtual bool ResourceAvailable() = 0;

  void IncreaseMemoryUsage(double resource) {
    DLOG(INFO) << "worker" << worker_id_ << " memory usage now is "
               << resource_usage_->GetMemory() << " and will increase by "
               << resource;
    resource_usage_->SetMemory(resource_usage_->GetMemory() + resource);
    CHECK(resource_usage_->GetMemory() <= resource_capacity_->GetMemory())
        << "memory usage is larger than capactiy!!!";
  }

  void DecreaseMemoryUsage(double resource) {
    DLOG(INFO) << "memory usage now is " << resource_usage_->GetMemory()
               << " and will decrease by " << resource;
    resource_usage_->SetMemory(resource_usage_->GetMemory() - resource);
    CHECK(resource_usage_->GetMemory() >= 0)
        << "memory usage is lower than 0!!!";
  }

  void IncreaseMemoryReservation(double resource) {
    resource_reservation_->SetMemory(resource_reservation_->GetMemory() +
                                     resource);
  }

  void DecreaseMemoryReservation(double resource) {
    resource_reservation_->SetMemory(resource_reservation_->GetMemory() -
                                     resource);
  }

  double RandomNoise(double duration) {
    auto seed =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto rand_noise =
        std::bind(std::uniform_real_distribution<double>(0, duration),
                  std::mt19937(seed));
    return rand_noise();
  }

  std::shared_ptr<ResourcePack> resource_capacity_;
  std::shared_ptr<ResourcePack> resource_usage_;
  std::shared_ptr<ResourcePack> resource_reservation_;
  int worker_id_;
  bool is_worker_;
};

} // namespace simulation
} //  namespace axe
