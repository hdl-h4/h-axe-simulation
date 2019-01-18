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
#include "job/shard_task.h"
#include "nlohmann/json.hpp"
#include "resource/resource.h"
#include <fstream>
#include <iomanip>
#include <vector>

namespace axe {
namespace simulation {

using nlohmann::json;

class Worker {
public:
  Worker(){};
  Worker(double cpu, double memory, double disk, double network)
      : resource_capacity_(cpu, memory, disk, network) {}

  friend void from_json(const json &j, Worker &worker) {
    j.get_to(worker.resource_capacity_);
  }

  inline auto GetRemainResourcePack() const {
    return resource_capacity_.Subtract(resource_usage_);
  }

  std::vector<double> GenerateUtilizationRecord(double time) {
    std::vector<double> record;
    record.push_back(time);
    for (int i = 0; i < kNumResourceTypes; ++i) {
      record.push_back(resource_usage_.GetResourceByIndex(i) /
                       resource_capacity_.GetResourceByIndex(i));
    }
    return record;
  }

  // task finish
  std::vector<ShardTask> TaskFinish(double time, const ShardTask &task) {
    std::vector<ShardTask> tasks;
    if (task.GetResourceType() == kCpu) {
      resource_usage_.SetCPU(resource_usage_.GetCPU() - task.GetReq());
      resource_reservation_.SetCPU(resource_reservation_.GetCPU() -
                                   task.GetReq());
      while (cpu_queue_.size() != 0 &&
             cpu_queue_.at(0).GetReq() + resource_usage_.GetCPU() <
                 resource_capacity_.GetCPU()) {
        tasks.push_back(cpu_queue_.at(0));
        resource_usage_.SetCPU(resource_usage_.GetCPU() +
                               cpu_queue_.at(0).GetReq());
        cpu_queue_.erase(cpu_queue_.begin());
      }
    } else if (task.GetResourceType() == kNetwork) {
      resource_usage_.SetNetwork(resource_usage_.GetNetwork() - task.GetReq());
      resource_reservation_.SetNetwork(resource_reservation_.GetNetwork() -
                                       task.GetReq());
      while (net_queue_.size() != 0 &&
             net_queue_.at(0).GetReq() + resource_usage_.GetNetwork() <
                 resource_capacity_.GetNetwork()) {
        tasks.push_back(net_queue_.at(0));
        resource_usage_.SetNetwork(resource_usage_.GetNetwork() +
                                   net_queue_.at(0).GetReq());
        net_queue_.erase(net_queue_.begin());
      }
    } else {
      resource_usage_.SetDisk(resource_usage_.GetDisk() - task.GetReq());
      resource_reservation_.SetDisk(resource_reservation_.GetDisk() -
                                    task.GetReq());
      while (disk_queue_.size() != 0 &&
             disk_queue_.at(0).GetReq() + resource_usage_.GetDisk() <
                 resource_capacity_.GetDisk()) {
        tasks.push_back(disk_queue_.at(0));
        resource_usage_.SetDisk(resource_usage_.GetDisk() +
                                disk_queue_.at(0).GetReq());
        disk_queue_.erase(disk_queue_.begin());
      }
    }
    records_.push_back(GenerateUtilizationRecord(time));
    return tasks;
  }

  // subgraph finish
  void SubGraphFinish(double time, double mem) {
    resource_usage_.SetMemory(resource_usage_.GetMemory() - mem);
    resource_reservation_.SetMemory(resource_reservation_.GetMemory() - mem);
    records_.push_back(GenerateUtilizationRecord(time));
  }

  // place new task, return true  : task runs;
  //                        false : task waits in queue;
  bool PlaceNewTask(double time, const ShardTask &task) {
    if (task.GetResourceType() == kCpu) {
      if (cpu_queue_.size() == 0 && resource_usage_.GetCPU() + task.GetReq() <
                                        resource_capacity_.GetCPU()) {
        resource_usage_.SetCPU(resource_usage_.GetCPU() + task.GetReq());
        records_.push_back(GenerateUtilizationRecord(time));
        return true;
      } else {
        cpu_queue_.push_back(task);
        return false;
      }
    } else if (task.GetResourceType() == kNetwork) {
      if (net_queue_.size() == 0 &&
          resource_usage_.GetNetwork() + task.GetReq() <
              resource_capacity_.GetNetwork()) {
        resource_usage_.SetNetwork(resource_usage_.GetNetwork() +
                                   task.GetReq());
        records_.push_back(GenerateUtilizationRecord(time));
        return true;
      } else {
        net_queue_.push_back(task);
        return false;
      }
    } else {
      if (disk_queue_.size() == 0 && resource_usage_.GetDisk() + task.GetReq() <
                                         resource_capacity_.GetDisk()) {
        resource_usage_.SetDisk(resource_usage_.GetDisk() + task.GetReq());
        records_.push_back(GenerateUtilizationRecord(time));
        return true;
      } else {
        disk_queue_.push_back(task);
        return false;
      }
    }
  }

  void Print() {
    DLOG(INFO) << "cpu: " << resource_capacity_.GetCPU();
    DLOG(INFO) << "mem: " << resource_capacity_.GetMemory();
    DLOG(INFO) << "disk: " << resource_capacity_.GetDisk();
    DLOG(INFO) << "net: " << resource_capacity_.GetNetwork();
  }

  void PrintReservation() {
    DLOG(INFO) << "cpu: " << resource_reservation_.GetCPU();
    DLOG(INFO) << "mem: " << resource_reservation_.GetMemory();
    DLOG(INFO) << "disk: " << resource_reservation_.GetDisk();
    DLOG(INFO) << "net: " << resource_reservation_.GetNetwork();
  }

  void ReportUtilization(std::ofstream &fout) {
    int time = 0;
    fout << "time(second)" << std::setw(15) << "CPU" << std::setw(10)
         << "MEMORY" << std::setw(10) << "DISK" << std::setw(10) << "NETWORK"
         << std::setw(10) << std::endl;
    for (auto const &record : records_) {
      while (time <= static_cast<int>(record[0])) {
        fout << time << std::setw(15);
        for (int i = 1; i < record.size(); ++i) {
          fout << record[i] << std::setw(10);
        }
        fout << std::endl;
        ++time;
      }
    }
  }

  void IncreaseMemoryUsage(double memory) {
    resource_usage_.SetMemory(resource_usage_.GetMemory() + memory);
  }

  bool Reserve(ResourcePack resource) {
    if (resource_capacity_.FitIn(resource_reservation_.Add(resource))) {
      resource_reservation_.AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

private:
  ResourcePack resource_capacity_;
  ResourcePack resource_usage_;
  ResourcePack resource_reservation_;
  std::vector<ShardTask> cpu_queue_;
  std::vector<ShardTask> disk_queue_;
  std::vector<ShardTask> net_queue_;
  std::vector<std::vector<double>> records_;
};

} //  namespace simulation
} //  namespace axe
