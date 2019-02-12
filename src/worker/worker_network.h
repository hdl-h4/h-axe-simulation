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
#include <cstdlib>
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

class WorkerNetwork : public WorkerCommon {
public:
  WorkerNetwork() {}
  WorkerNetwork(int worker_id, std::shared_ptr<ResourcePack> resource_capacity,
                std::shared_ptr<ResourcePack> resource_usage,
                std::shared_ptr<ResourcePack> resource_reservation,
                std::shared_ptr<std::set<int>> invalid_event_id_set,
                bool is_worker)
      : WorkerCommon(worker_id, resource_capacity, resource_usage,
                     resource_reservation, is_worker),
        invalid_event_id_set_(invalid_event_id_set) {
    network_slot_ =
        resource_capacity_->GetNetwork() / maximum_network_task_number_;
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
    DeleteRunningTask(event_id);
    DecreaseNetworkUsage(network_slot_);
    DecreaseNetworkReservation(task.GetReq());
    if (network_queue_.size() != 0 && ResourceAvailable()) {
      IncreaseNetworkUsage(network_slot_);
      if (network_queue_.at(0).GetMemory() > 0) {
        IncreaseMemoryUsage(network_queue_.at(0).GetMemory());
      }
      ShardTask task = network_queue_.at(0);
      double duration = ComputeNewTaskDuration(task);
      std::shared_ptr<TaskFinishEvent> task_finish_event =
          std::make_shared<TaskFinishEvent>(
              TaskFinishEvent(EventType::TASK_FINISH, time + duration, 0,
                              task.GetJobID(), task));
      event_vector.push_back(task_finish_event);
      AddRunningTaskRecord(time, task_finish_event->GetEventID(), task);
      network_queue_.erase(network_queue_.begin());
    } else {
      std::vector<std::shared_ptr<Event>> update_event_vector =
          UpdateRunningTask(time);
      event_vector.insert(event_vector.end(), update_event_vector.begin(),
                          update_event_vector.end());
    }
    return event_vector;
  }

  std::vector<std::shared_ptr<Event>> PlaceNewTask(double time,
                                                   const ShardTask &task) {
    std::vector<std::shared_ptr<Event>> event_vector;

    if (network_queue_.size() == 0 && ResourceAvailable()) {
      IncreaseNetworkUsage(network_slot_);
      if (task.GetMemory() > 0) {
        IncreaseMemoryUsage(task.GetMemory());
      }
      double duration = ComputeNewTaskDuration(task);
      std::shared_ptr<TaskFinishEvent> task_finish_event =
          std::make_shared<TaskFinishEvent>(
              TaskFinishEvent(EventType::TASK_FINISH, time + duration, 0,
                              task.GetJobID(), task));
      event_vector.push_back(task_finish_event);
      std::vector<std::shared_ptr<Event>> update_event_vector =
          AddRunningTaskRecord(time, task_finish_event->GetEventID(), task);
      event_vector.insert(event_vector.end(), update_event_vector.begin(),
                          update_event_vector.end());
    } else {
      network_queue_.push_back(task);
    }
    DLOG(INFO) << "event vector size: " << event_vector.size();
    return event_vector;
  }

private:
  void
  SetInvalidEventIDSet(std::shared_ptr<std::set<int>> invalid_event_id_set) {
    invalid_event_id_set_ = invalid_event_id_set;
  }

  void IncreaseNetworkUsage(double resource) {
    resource_usage_->SetNetwork(resource_usage_->GetNetwork() + resource);
    ++current_network_task_number_;
  }

  void DecreaseNetworkUsage(double resource) {
    resource_usage_->SetNetwork(resource_usage_->GetNetwork() - resource);
    --current_network_task_number_;
  }

  void IncreaseNetworkReservation(double resource) {
    if (is_worker_)
      resource_reservation_->SetNetwork(resource_reservation_->GetNetwork() +
                                        resource);
  }

  void DecreaseNetworkReservation(double resource) {
    if (is_worker_)
      resource_reservation_->SetNetwork(resource_reservation_->GetNetwork() -
                                        resource);
  }

  bool ResourceAvailable() {
    return current_network_task_number_ + 1 <= maximum_network_task_number_;
  }

  double ComputeNewTaskDuration(const ShardTask &task) {
    double duration = current_network_task_number_ * task.GetReq() /
                      resource_capacity_->GetNetwork();
    return duration + RandomNoise(duration);
  }

  double GetBandWidth() {
    return resource_capacity_->GetNetwork() / current_network_task_number_;
  }

  std::vector<std::shared_ptr<Event>> UpdateRunningTask(double time) {
    std::vector<std::shared_ptr<Event>> event_vector;
    for (auto &record : network_running_task_record_vector_) {
      if (fabs(record.last_check_bandwidth - GetBandWidth()) < 1e-6)
        continue;
      DLOG(INFO) << "bandwidth for job: " << record.shard_task.GetJobID()
                 << " task: " << record.shard_task.GetTaskID()
                 << " shard: " << record.shard_task.GetShardID()
                 << " change from " << record.last_check_bandwidth << " to "
                 << GetBandWidth();
      record.last_check_data_size -=
          record.last_check_bandwidth * (time - record.last_check_time);
      record.last_check_bandwidth = GetBandWidth();
      record.last_check_time = time;
      // make the event id of the old TaskFinishEvent invalid
      invalid_event_id_set_->insert(record.event_id);
      std::shared_ptr<TaskFinishEvent> new_task_finish_event =
          std::make_shared<TaskFinishEvent>(TaskFinishEvent(
              EventType::TASK_FINISH,
              time + record.last_check_data_size / record.last_check_bandwidth,
              0, record.shard_task.GetJobID(), record.shard_task));
      record.event_id = new_task_finish_event->GetEventID();
      event_vector.push_back(new_task_finish_event);
    }
    return event_vector;
  }

  std::vector<std::shared_ptr<Event>>
  AddRunningTaskRecord(double time, int event_id, const ShardTask &task) {
    std::vector<std::shared_ptr<Event>> event_vector;
    DiskNetworkRunningTaskRecord record = {event_id, task, time, task.GetReq(),
                                           GetBandWidth()};
    DLOG(INFO) << "(network) job: " << task.GetJobID()
               << " task: " << task.GetTaskID()
               << " shard: " << task.GetShardID()
               << " is running, the bandwidth is " << GetBandWidth();
    network_running_task_record_vector_.push_back(record);
    event_vector = UpdateRunningTask(time);
    return event_vector;
  }

  void DeleteRunningTask(int event_id) {
    for (auto iter = network_running_task_record_vector_.begin();
         iter != network_running_task_record_vector_.end(); ++iter) {
      if (iter->event_id == event_id) {
        DLOG(INFO) << "(network) job: " << iter->shard_task.GetJobID()
                   << " task: " << iter->shard_task.GetTaskID()
                   << " shard: " << iter->shard_task.GetShardID()
                   << " is over, the bandwidth before that was "
                   << GetBandWidth();
        DLOG(INFO) << "event id: " << iter->event_id << " is invalid now";
        network_running_task_record_vector_.erase(iter);
        return;
      }
    }
  }

  int maximum_network_task_number_ = 10; // the maximum number of network tasks
                                         // running on this worker
  int current_network_task_number_ =
      0; // the number of current running network tasks
  double network_slot_;
  std::vector<ShardTask> network_queue_;
  std::vector<DiskNetworkRunningTaskRecord> network_running_task_record_vector_;
  std::shared_ptr<std::set<int>> invalid_event_id_set_;
};

} //  namespace simulation
} //  namespace axe
