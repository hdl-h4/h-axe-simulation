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

class WorkerAbstract {
public:
  virtual void Init(int worker_id,
                    std::shared_ptr<std::set<int>> invalid_event_id_set) = 0;

  // place new task, return true  : task runs;
  //                        false : task waits in queue;
  std::vector<std::shared_ptr<Event>> PlaceNewTask(double time,
                                                   const ShardTask &task) {
    DLOG(INFO) << "worker " << worker_id_ << " place new task";
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
    records_.insert(GenerateUtilizationRecord(time));
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
    records_.insert(GenerateUtilizationRecord(time));
    return event_vector;
  }

  virtual void Print() = 0;

  std::pair<int, std::vector<double>> GenerateUtilizationRecord(double time) {

    std::vector<double> resource_vector;
    for (int i = 0; i < kNumResourceTypes; ++i) {
      CHECK(resource_usage_->GetResourceByIndex(i) >= 0)
          << "resource usage cannot be lower than 0";
      resource_vector.push_back(resource_usage_->GetResourceByIndex(i) /
                                resource_capacity_->GetResourceByIndex(i));
    }
    std::pair<int, std::vector<double>> record(static_cast<int>(time * 20),
                                               resource_vector);
    return record;
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
    for (auto iter = records_.begin(); iter != records_.end(); ++iter) {
      int time = iter->first;
      std::vector<double> record = iter->second;
      // fout << time << "\t";
      for (int i = 0; i < record.size(); ++i) {
        fout << record[i];
        if (i < record.size() - 1)
          fout << "\t";
      }
      fout << std::endl;
      auto next_iter = std::next(iter, 1);
      if (next_iter != records_.end()) {
        ++time;
        while (time < next_iter->first) {
          // fout << time << "\t";
          for (int i = 0; i < record.size(); ++i) {
            fout << record[i];
            if (i < record.size() - 1)
              fout << "\t";
          }
          fout << std::endl;
          ++time;
        }
      }
    }
  }

  virtual bool Reserve(ResourcePack resource) = 0;

  virtual bool TryToReserve(ResourcePack resource) = 0;

protected:
  std::map<int, std::vector<double>> records_;
  std::shared_ptr<std::set<int>> invalid_event_id_set_;
  int worker_id_;

  std::shared_ptr<ResourcePack> resource_capacity_;
  std::shared_ptr<ResourcePack> resource_usage_;
  std::shared_ptr<ResourcePack> resource_reservation_;

  WorkerCPU worker_cpu_;
  WorkerDisk worker_disk_;
  WorkerNetwork worker_network_;
};

} //  namespace simulation
} //  namespace axe
