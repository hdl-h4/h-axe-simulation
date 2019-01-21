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
  Worker() { oversell_factor_ = 1.5; };

  Worker(double cpu, double memory, double disk, double network)
      : resource_capacity_(cpu, memory, disk, network) {
    oversell_factor_ = 1.5;
  }

  friend void from_json(const json &j, Worker &worker) {
    j.get_to(worker.resource_capacity_);
    worker.records_.push_back(worker.GenerateUtilizationRecord(0));
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
    if (task.GetMemory() < 0) {
      IncreaseSingleResourceUsage(task.GetMemory(), static_cast<size_t>(ResourceType::kMemory));
    }
    if (task.GetResourceType() == static_cast<size_t>(ResourceType::kCPU)) {
      DecreaseSingleResourceUsage(task.GetReq(), static_cast<size_t>(ResourceType::kCPU));
      DecreaseSingleResourceReservation(task.GetReq(), static_cast<size_t>(ResourceType::kCPU));
      while (cpu_queue_.size() != 0 &&
             cpu_queue_.at(0).GetReq() + resource_usage_.GetCPU() <
                 resource_capacity_.GetCPU()) {
        tasks.push_back(cpu_queue_.at(0));
        IncreaseSingleResourceUsage(cpu_queue_.at(0).GetReq(), static_cast<size_t>(ResourceType::kCPU));
        cpu_queue_.erase(cpu_queue_.begin());
      }
    } else if (task.GetResourceType() == static_cast<size_t>(ResourceType::kNetwork)) {
      DecreaseSingleResourceUsage(task.GetReq(), static_cast<size_t>(ResourceType::kNetwork));
      DecreaseSingleResourceReservation(task.GetReq(), static_cast<size_t>(ResourceType::kNetwork));
      while (net_queue_.size() != 0 &&
             net_queue_.at(0).GetReq() + resource_usage_.GetNetwork() <
                 resource_capacity_.GetNetwork()) {
        tasks.push_back(net_queue_.at(0));
        IncreaseSingleResourceUsage(net_queue_.at(0).GetReq(), static_cast<size_t>(ResourceType::kNetwork));
        net_queue_.erase(net_queue_.begin());
      }
    } else if (task.GetResourceType() == static_cast<size_t>(ResourceType::kDisk)) {
      DecreaseSingleResourceUsage(task.GetReq(), static_cast<size_t>(ResourceType::kDisk));
      DecreaseSingleResourceReservation(task.GetReq(), static_cast<size_t>(ResourceType::kDisk));
      while (disk_queue_.size() != 0 &&
             disk_queue_.at(0).GetReq() + resource_usage_.GetDisk() <
                 resource_capacity_.GetDisk()) {
        tasks.push_back(disk_queue_.at(0));
        IncreaseSingleResourceUsage(disk_queue_.at(0).GetReq(), static_cast<size_t>(ResourceType::kDisk));
        disk_queue_.erase(disk_queue_.begin());
      }
    } else {
      CHECK(true) << "invalid resource type";
    }
    records_.push_back(GenerateUtilizationRecord(time));
    return tasks;
  }

  // subgraph finish
  void SubGraphFinish(double time, double mem) {
    DecreaseSingleResourceReservation(mem, static_cast<size_t>(ResourceType::kMemory));
    records_.push_back(GenerateUtilizationRecord(time));
  }

  // place new task, return true  : task runs;
  //                        false : task waits in queue;
  bool PlaceNewTask(double time, const ShardTask &task) {
    if (task.GetMemory() > 0) {
      IncreaseSingleResourceUsage(task.GetMemory(), static_cast<size_t>(ResourceType::kMemory));
    }
    if (task.GetResourceType() == static_cast<size_t>(ResourceType::kCPU)) {
      if (cpu_queue_.size() == 0 && resource_usage_.GetCPU() + task.GetReq() <
                                        resource_capacity_.GetCPU()) {
        IncreaseSingleResourceUsage(task.GetReq(), static_cast<size_t>(ResourceType::kCPU));
        records_.push_back(GenerateUtilizationRecord(time));
        return true;
      } else {
        cpu_queue_.push_back(task);
        return false;
      }
    } else if (task.GetResourceType() == static_cast<size_t>(ResourceType::kNetwork)) {
      if (net_queue_.size() == 0 &&
          resource_usage_.GetNetwork() + task.GetReq() <
              resource_capacity_.GetNetwork()) {
        IncreaseSingleResourceUsage(task.GetReq(), static_cast<size_t>(ResourceType::kNetwork));
        records_.push_back(GenerateUtilizationRecord(time));
        return true;
      } else {
        net_queue_.push_back(task);
        return false;
      }
    } else {
      if (disk_queue_.size() == 0 && resource_usage_.GetDisk() + task.GetReq() <
                                         resource_capacity_.GetDisk()) {
        IncreaseSingleResourceUsage(task.GetReq(), static_cast<size_t>(ResourceType::kDisk));
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
    for (int i = 0; i < records_.size(); ++i) {
      auto &this_record = records_[i];
      fout << time << std::setw(15);
      for (int j = 1; j < this_record.size(); ++j) {
        fout << this_record[j] << std::setw(10);
      }
      fout << std::endl;
      if (i < records_.size() - 1) {
        auto &next_record = records_[i + 1];
        ++time;
        while (time < static_cast<int>(next_record[0])) {
          fout << time << std::setw(15);
          for (int j = 1; j < this_record.size(); ++j) {
            fout << this_record[j] << std::setw(10);
          }
          fout << std::endl;
          ++time;
        }
      }
    }
  }

  void IncreaseSingleResourceUsage(double resource, int type) {
    switch (type) {
    case static_cast<size_t>(ResourceType::kCPU):
      resource_usage_.SetCPU(resource_usage_.GetCPU() + resource);
      break;
    case static_cast<size_t>(ResourceType::kMemory):
      resource_usage_.SetMemory(resource_usage_.GetMemory() + resource);
      break;
    case static_cast<size_t>(ResourceType::kDisk):
      resource_usage_.SetDisk(resource_usage_.GetDisk() + resource);
      break;
    case static_cast<size_t>(ResourceType::kNetwork):
      resource_usage_.SetNetwork(resource_usage_.GetNetwork() + resource);
      break;
    default:
      CHECK(true) << "fault resource type";
    }
  }

  void DecreaseSingleResourceUsage(double resource, int type) {
    switch (type) {
    case static_cast<size_t>(ResourceType::kCPU):
      resource_usage_.SetCPU(resource_usage_.GetCPU() - resource);
      break;
    case static_cast<size_t>(ResourceType::kMemory):
      resource_usage_.SetMemory(resource_usage_.GetMemory() - resource);
      break;
    case static_cast<size_t>(ResourceType::kDisk):
      resource_usage_.SetDisk(resource_usage_.GetDisk() - resource);
      break;
    case static_cast<size_t>(ResourceType::kNetwork):
      resource_usage_.SetNetwork(resource_usage_.GetNetwork() - resource);
      break;
    default:
      CHECK(true) << "fault resource type";
    }
  }

  void IncreaseSingleResourceReservation(double resource, int type) {
    switch (type) {
    case static_cast<size_t>(ResourceType::kCPU):
      resource_reservation_.SetCPU(resource_reservation_.GetCPU() + resource);
      break;
    case static_cast<size_t>(ResourceType::kMemory):
      resource_reservation_.SetMemory(resource_reservation_.GetMemory() +
                                      resource);
      break;
    case static_cast<size_t>(ResourceType::kDisk):
      resource_reservation_.SetDisk(resource_reservation_.GetDisk() + resource);
      break;
    case static_cast<size_t>(ResourceType::kNetwork):
      resource_reservation_.SetNetwork(resource_reservation_.GetNetwork() +
                                       resource);
      break;
    default:
      CHECK(true) << "fault resource type";
    }
  }

  void DecreaseSingleResourceReservation(double resource, int type) {
    switch (type) {
    case static_cast<size_t>(ResourceType::kCPU):
      resource_reservation_.SetCPU(resource_reservation_.GetCPU() - resource);
      break;
    case static_cast<size_t>(ResourceType::kMemory):
      resource_reservation_.SetMemory(resource_reservation_.GetMemory() -
                                      resource);
      break;
    case static_cast<size_t>(ResourceType::kDisk):
      resource_reservation_.SetDisk(resource_reservation_.GetDisk() - resource);
      break;
    case static_cast<size_t>(ResourceType::kNetwork):
      resource_reservation_.SetNetwork(resource_reservation_.GetNetwork() -
                                       resource);
      break;
    default:
      CHECK(true) << "fault resource type";
    }
  }

  bool Reserve(ResourcePack resource) {
    if (resource_capacity_.FitIn(resource_reservation_.Add(resource))) {
      resource_reservation_.AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

  bool WeakReserve(ResourcePack resource) {
    if (resource_capacity_.WeakFitIn(resource_reservation_.Add(resource),
                                     oversell_factor_)) {
      resource_reservation_.AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

  bool TryToReserve(ResourcePack resource) {
    return resource_capacity_.FitIn(resource_reservation_.Add(resource));
  }

  ResourcePack GetAvailableResource() {
    return resource_capacity_.Subtract(resource_usage_);
  }

  double ComputeDotProduct(ResourcePack resource) {
    double product = 0;
    ResourcePack resource_available = GetAvailableResource();
    for (int i = 0; i < kNumResourceTypes; ++i) {
      product += resource.GetResourceByIndex(i) *
                 resource_available.GetResourceByIndex(i);
    }
    return product;
  }

private:
  ResourcePack resource_capacity_;
  ResourcePack resource_usage_;
  ResourcePack resource_reservation_;
  std::vector<ShardTask> cpu_queue_;
  std::vector<ShardTask> disk_queue_;
  std::vector<ShardTask> net_queue_;
  std::vector<std::vector<double>> records_;
  double oversell_factor_;
};

} //  namespace simulation
} //  namespace axe
