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
#include "worker_cpu.h"
#include "worker_disk.h"
#include "worker_network.h"

namespace axe {
namespace simulation {

using nlohmann::json;

class Worker {
public:
  Worker() {}

  Worker(double cpu, double memory, double disk, double network) {
    // CHECK(false) << "don't support this worker constructor now";
    resource_capacity_ =
        std::make_shared<ResourcePack>(cpu, memory, disk, network);
    resource_usage_ = std::make_shared<ResourcePack>();
    resource_reservation_ = std::make_shared<ResourcePack>();
    resource_maximum_reservation_ = std::make_shared<ResourcePack>(
        30, resource_capacity_->GetMemory(), 2000, 2000);
  }

  void Init(std::shared_ptr<std::set<int>> invalid_event_id_set) {
    invalid_event_id_set_ = invalid_event_id_set;
    worker_cpu_ =
        WorkerCPU(resource_capacity_, resource_usage_, resource_reservation_);
    worker_disk_ = WorkerDisk(resource_capacity_, resource_usage_,
                              resource_reservation_, invalid_event_id_set_);
    worker_network_ =
        WorkerNetwork(resource_capacity_, resource_usage_,
                      resource_reservation_, invalid_event_id_set_);
  }

  friend void from_json(const json &j, Worker &worker) {
    worker.resource_capacity_ = std::make_shared<ResourcePack>();
    worker.resource_usage_ = std::make_shared<ResourcePack>();
    worker.resource_reservation_ = std::make_shared<ResourcePack>();
    worker.resource_maximum_reservation_ = std::make_shared<ResourcePack>();
    j.get_to(*(worker.resource_capacity_));
    worker.records_.push_back(worker.GenerateUtilizationRecord(0));
    *(worker.resource_maximum_reservation_) = {
        30, worker.resource_capacity_->GetMemory(), 2000, 2000};
  }

  inline auto GetRemainResourcePack() const {
    return resource_capacity_->Subtract(*resource_usage_);
  }

  std::vector<double> GenerateUtilizationRecord(double time) {
    std::vector<double> record;
    record.push_back(time);
    for (int i = 0; i < kNumResourceTypes; ++i) {
      CHECK(resource_usage_->GetResourceByIndex(i) >= 0)
          << "resource usage cannot be lower than 0";
      record.push_back(resource_usage_->GetResourceByIndex(i) /
                       resource_capacity_->GetResourceByIndex(i));
    }
    return record;
  }

  // place new task, return true  : task runs;
  //                        false : task waits in queue;
  std::vector<std::shared_ptr<Event>> PlaceNewTask(double time,
                                                   const ShardTask &task) {
    std::vector<std::shared_ptr<Event>> event_vector;
    if (task.GetResourceType() == ResourceType::kCPU) {
      event_vector = worker_cpu_.PlaceNewTask(time, task);
    } else if (task.GetResourceType() == ResourceType::kDisk) {
      event_vector = worker_disk_.PlaceNewTask(time, task);
    } else if (task.GetResourceType() == ResourceType::kNetwork) {
      event_vector = worker_network_.PlaceNewTask(time, task);
    } else {
      CHECK(false) << "invalid resource type";
    }
    records_.push_back(GenerateUtilizationRecord(time));
    return event_vector;
  }

  // task finish
  std::vector<std::shared_ptr<Event>> TaskFinish(double time, int event_id,
                                                 const ShardTask &task) {
    std::vector<std::shared_ptr<Event>> event_vector;
    if (task.GetResourceType() == ResourceType::kCPU) {
      event_vector = worker_cpu_.TaskFinish(time, event_id, task);
    } else if (task.GetResourceType() == ResourceType::kDisk) {
      event_vector = worker_disk_.TaskFinish(time, event_id, task);
    } else if (task.GetResourceType() == ResourceType::kNetwork) {
      event_vector = worker_network_.TaskFinish(time, event_id, task);
    } else {
      CHECK(false) << "invalid resource type";
    }
    records_.push_back(GenerateUtilizationRecord(time));
    return event_vector;
  }

  // subgraph finish
  void SubGraphFinish(double time, double mem) {
    resource_reservation_->SetMemory(resource_reservation_->GetMemory() - mem);
    records_.push_back(GenerateUtilizationRecord(time));
  }

  void Print() {
    DLOG(INFO) << "cpu: " << resource_capacity_->GetCPU();
    DLOG(INFO) << "mem: " << resource_capacity_->GetMemory();
    DLOG(INFO) << "disk: " << resource_capacity_->GetDisk();
    DLOG(INFO) << "net: " << resource_capacity_->GetNetwork();
  }

  void PrintReservation() {
    DLOG(INFO) << "cpu: " << resource_reservation_->GetCPU();
    DLOG(INFO) << "mem: " << resource_reservation_->GetMemory();
    DLOG(INFO) << "disk: " << resource_reservation_->GetDisk();
    DLOG(INFO) << "net: " << resource_reservation_->GetNetwork();
  }

  void ReportUtilization(std::ofstream &fout) {
    int time = 0;
    fout << "#CPU"
         << "\t"
         << "MEMORY"
         << "\t"
         << "DISK"
         << "\t"
         << "NETWORK" << std::endl;
    for (int i = 0; i < records_.size(); ++i) {
      auto &this_record = records_[i];
      for (int j = 1; j < this_record.size(); ++j) {
        fout << this_record[j];
        if (j < kNumResourceTypes)
          fout << "\t";
      }
      fout << std::endl;
      if (i < records_.size() - 1) {
        auto &next_record = records_[i + 1];
        ++time;
        while (time < static_cast<int>(next_record[0])) {
          for (int j = 1; j < this_record.size(); ++j) {
            fout << this_record[j];
            if (j < kNumResourceTypes)
              fout << "\t";
          }
          fout << std::endl;
          ++time;
        }
      }
    }
  }

  bool Reserve(ResourcePack resource) {
    if (resource_reservation_->Add(resource).FitIn(
            *resource_maximum_reservation_)) {
      resource_reservation_->AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

  bool WeakReserve(ResourcePack resource) {
    if (resource_reservation_->Add(resource).WeakFitIn(
            *resource_maximum_reservation_, oversell_factor_)) {
      resource_reservation_->AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

  bool TryToReserve(ResourcePack resource) {
    return resource_maximum_reservation_->FitIn(
        resource_reservation_->Add(resource));
  }

  ResourcePack GetAvailableResource() {
    return resource_capacity_->Subtract(*resource_usage_);
  }

private:
  std::shared_ptr<ResourcePack> resource_capacity_;
  /* Capacity: capacity describes the physical resources on the worker.
   *   CPU: the number of cpu cores on this worker (e.g., 8 cores).
   *   Memory: the size of memory on this worker (e.g., 16GB).
   *   Disk: the bandwidth of disk on this worker (e.g., 100MB/s).
   *   Network: the bandwidth of network on this worker (e.g., 1GB/s).
   */
  std::shared_ptr<ResourcePack> resource_usage_;
  /* Usage: usage describes the resource utilization at this moment which is
   * considered in local queue management.
   *    CPU: the number of cpu cores in use on this worker (e.g., 4 cores in
   * use).
   *    Memory: the size of memory in use on this worker (e.g., 10GB in use).
   *    Disk: the share of disk bandwidth on the worker (e.g., 75MB/s if you
   * have 3 disk tasks sharing a 100MB/s bandwidth and the maximum disk task
   * number is 4).
   *    Network: the share of network bandwidth on this worker (e.g.,
   * 0.5GB/s if you have 5 network tasks sharing a 1GB/s bandwidth and the
   * maximum network task number is 10).
   */
  std::shared_ptr<ResourcePack> resource_reservation_;
  /* Reservation: reservation is a coarse description of the expected workload
   * on this worker which is considered in global scheduling.
   *    CPU: the total CPU workload reserved on this worker (the total workload
   * means the accumulation of cpu work time of every cpu task).
   *    Memory: the size of memory reserved on this worker (e.g., 10 GB in
   * total and 8GB reserved).
   *    Disk: the total disk workload reserved on this worker (the total
   * workload means the accumulation of data size of every disk task).
   *    Network: the total network workload reserved on this worker (same as
   * Disk).
   */
  std::shared_ptr<ResourcePack> resource_maximum_reservation_;
  // the maximum reservation which is a upper bound during Reserve()

  std::vector<std::vector<double>> records_;
  double oversell_factor_ = 1.5;
  std::shared_ptr<std::set<int>> invalid_event_id_set_;
  WorkerCPU worker_cpu_;
  WorkerDisk worker_disk_;
  WorkerNetwork worker_network_;
};

} //  namespace simulation
} //  namespace axe
