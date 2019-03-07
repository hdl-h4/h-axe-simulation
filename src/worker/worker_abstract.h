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
                    std::shared_ptr<std::set<int>> invalid_event_id_set) {}

  virtual void Print() {}

  virtual void PrintReservation() {}

  virtual bool WeakReserve(ResourcePack resource) {}

  virtual bool Reserve(ResourcePack resource) {}

  virtual bool TryToReserve(ResourcePack resource) {}

  virtual void SubGraphFinish(double time, ResourcePack resource) {}

  /*
  friend void from_json(const json& j, WorkerAbstract &worker_abstract) {
    worker_abstract.resource_capacity_ = std::make_shared<ResourcePack>();
    worker_abstract.resource_usage_ = std::make_shared<ResourcePack>();
    worker_abstract.resource_reservation_ = std::make_shared<ResourcePack>();
    //j.get_to(*(worker_abstract.resource_capacity_));
  }

  void ReadFromJson(const json& j) {
    resource_capacity_ = std::make_shared<ResourcePack>();
    resource_usage_ = std::make_shared<ResourcePack>();
    resource_reservation_ = std::make_shared<ResourcePack>();
    j.get_to(*(resource_capacity_));
  }
  */

  const auto &GetAverageUsage() { return average_usage_; }
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

  std::pair<int, std::vector<double>> GenerateUtilizationRecord(double time) {

    std::vector<double> resource_vector;
    for (int i = 0; i < kNumResourceTypes; ++i) {
      if (resource_usage_->GetResourceByIndex(i) < 0 &&
          -resource_usage_->GetResourceByIndex(i) < (double)1e-6) {
        resource_usage_->SetResourceByIndex(i, 0);
      }
      CHECK(resource_usage_->GetResourceByIndex(i) >= 0)
          << "resource usage cannot be lower than 0";
      resource_vector.push_back(resource_usage_->GetResourceByIndex(i) /
                                resource_capacity_->GetResourceByIndex(i));
    }
    std::pair<int, std::vector<double>> record(static_cast<int>(time),
                                               resource_vector);
    return record;
  }

  void ReportUtilization(std::ofstream &fout,
                         std::vector<std::vector<double>> &total_records_) {
    int time = 0;
    fout << "#CPU"
         << "\t"
         << "MEMORY"
         << "\t"
         << "DISK"
         << "\t"
         << "NETWORK" << std::endl;

    for (int i = 0; i < kNumResourceTypes; i++) {
      average_usage_.push_back(0);
    }

    int idx = 0;

    for (auto iter = records_.begin(); iter != records_.end(); ++iter) {
      int time = iter->first;
      std::vector<double> record = iter->second;
      // fout << time << "\t";
      for (int i = 0; i < record.size(); ++i) {
        if (time < 100000)
          total_records_.at(time).at(i) += record[i];
        average_usage_[i] += record[i];
        fout << record[i];
        if (i < record.size() - 1)
          fout << "\t";
      }
      idx++;
      fout << std::endl;
      auto next_iter = std::next(iter, 1);
      if (next_iter != records_.end()) {
        ++time;
        while (time < next_iter->first) {
          // fout << time << "\t";
          for (int i = 0; i < record.size(); ++i) {
            if (time < 100000)
              total_records_.at(time).at(i) += record[i];
            fout << record[i];
            if (i < record.size() - 1)
              fout << "\t";
          }
          idx++;
          fout << std::endl;
          ++time;
        }
      }
    }
    for (int i = 0; i < kNumResourceTypes; i++) {
      average_usage_[i] /= records_.size();
    }
  }

  inline auto GetRemainResourcePack() const {
    return resource_capacity_->Subtract(*resource_usage_);
  }

  ResourcePack GetAvailableResource() {
    return resource_capacity_->Subtract(*resource_usage_);
  }

protected:
  std::map<int, std::vector<double>> records_;
  std::vector<double> average_usage_;
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
