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

#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
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
  WorkerCommon(std::shared_ptr<ResourcePack> resource_capacity,
               std::shared_ptr<ResourcePack> resource_usage,
               std::shared_ptr<ResourcePack> resource_reservation)
      : resource_capacity_(resource_capacity), resource_usage_(resource_usage),
        resource_reservation_(resource_reservation) {}

  virtual std::vector<std::shared_ptr<Event>>
  PlaceNewTask(double time, const ShardTask &task) = 0;

  virtual std::vector<std::shared_ptr<Event>>
  TaskFinish(double time, int event_id, const ShardTask &task) = 0;

protected:
  virtual double ComputeNewTaskDuration(const ShardTask &task) = 0;
  virtual bool ResourceAvailable() = 0;

  void IncreaseMemoryUsage(double resource) {
    resource_usage_->SetMemory(resource_usage_->GetMemory() + resource);
  }

  void DecreaseMemoryUsage(double resource) {
    resource_usage_->SetMemory(resource_usage_->GetMemory() - resource);
  }

  void IncreaseMemoryReservation(double resource) {
    resource_reservation_->SetMemory(resource_reservation_->GetMemory() +
                                     resource);
  }

  void DecreaseMemoryReservation(double resource) {
    resource_reservation_->SetMemory(resource_reservation_->GetMemory() -
                                     resource);
  }
  std::shared_ptr<ResourcePack> resource_capacity_;
  std::shared_ptr<ResourcePack> resource_usage_;
  std::shared_ptr<ResourcePack> resource_reservation_;
};

} // namespace simulation
} //  namespace axe
