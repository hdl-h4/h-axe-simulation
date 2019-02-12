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

class WorkerCPU : public WorkerCommon {
public:
  WorkerCPU() {}
  WorkerCPU(int worker_id, std::shared_ptr<ResourcePack> resource_capacity,
            std::shared_ptr<ResourcePack> resource_usage,
            std::shared_ptr<ResourcePack> resource_reservation, bool is_worker)
      : WorkerCommon(worker_id, resource_capacity, resource_usage,
                     resource_reservation, is_worker) {}

  std::vector<std::shared_ptr<Event>> PlaceNewTask(double time,
                                                   const ShardTask &task) {
    std::vector<std::shared_ptr<Event>> event_vector;

    if (cpu_queue_.size() == 0 && ResourceAvailable()) {
      IncreaseCPUUsage(1);
      if (task.GetMemory() > 0) {
        IncreaseMemoryUsage(task.GetMemory());
      }

      double duration = ComputeNewTaskDuration(task);
      std::shared_ptr<TaskFinishEvent> task_finish_event =
          std::make_shared<TaskFinishEvent>(
              TaskFinishEvent(EventType::TASK_FINISH, time + duration, 0,
                              task.GetJobID(), task));
      event_vector.push_back(task_finish_event);
    } else {
      cpu_queue_.push_back(task);
    }
    return event_vector;
  }

  std::vector<std::shared_ptr<Event>> TaskFinish(double time, int event_id,
                                                 const ShardTask &task) {
    std::vector<std::shared_ptr<Event>> event_vector;
    DLOG(INFO) << "job:task:shard " << task.GetJobID() << ' '
               << task.GetTaskID() << ' ' << task.GetShardID() << " memory is "
               << task.GetMemory();
    if (task.GetMemory() < 0) {
      DLOG(INFO) << "will release memory " << task.GetMemory();
      IncreaseMemoryUsage(task.GetMemory());
    }
    DecreaseCPUUsage(1);
    DecreaseCPUReservation(task.GetReq());
    if (cpu_queue_.size() != 0 && ResourceAvailable()) {
      IncreaseCPUUsage(1);
      if (cpu_queue_.at(0).GetMemory() > 0) {
        IncreaseMemoryUsage(cpu_queue_.at(0).GetMemory());
      }

      ShardTask task = cpu_queue_.at(0);
      double duration = ComputeNewTaskDuration(task);
      std::shared_ptr<TaskFinishEvent> task_finish_event =
          std::make_shared<TaskFinishEvent>(
              TaskFinishEvent(EventType::TASK_FINISH, time + duration, 0,
                              task.GetJobID(), task));
      event_vector.push_back(task_finish_event);
      cpu_queue_.erase(cpu_queue_.begin());
    }
    return event_vector;
  }

private:
  void IncreaseCPUUsage(double resource) {
    resource_usage_->SetCPU(resource_usage_->GetCPU() + resource);
  }

  void DecreaseCPUUsage(double resource) {
    resource_usage_->SetCPU(resource_usage_->GetCPU() - resource);
  }

  void IncreaseCPUReservation(double resource) {
    if (is_worker_)
      resource_reservation_->SetCPU(resource_reservation_->GetCPU() + resource);
  }

  void DecreaseCPUReservation(double resource) {
    if (is_worker_)
      resource_reservation_->SetCPU(resource_reservation_->GetCPU() - resource);
  }

  double ComputeNewTaskDuration(const ShardTask &task) {
    double duration = task.GetReq();
    return duration + RandomNoise(duration);
  }

  bool ResourceAvailable() {
    return static_cast<int>(resource_usage_->GetCPU()) + 1 <=
           static_cast<int>(resource_capacity_->GetCPU());
  }

  std::vector<ShardTask> cpu_queue_;
};

} // namespace simulation
} //  namespace axe
