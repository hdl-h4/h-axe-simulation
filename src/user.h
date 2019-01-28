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

namespace axe {
namespace simulation {

class User {
public:
  User() {}

  void SetClusterResourceCapacity(const ResourcePack &resource_pack) {
    cluster_resource_capacity_ = resource_pack;
    records_.push_back(GenerateResourceReservationRecord(0));
  }

  inline const auto &GetResourceReservation() { return resource_reservation_; }
  inline const auto &GetJobsID() { return jobs_id_; }
  inline const auto &GetRecords() { return records_; }

  void AddJobID(int job_id) { jobs_id_.push_back(job_id); }

  void PlacementDecision(double time, const ResourcePack &resource) {
    resource_reservation_.AddToMe(resource);
    records_.push_back(GenerateResourceReservationRecord(time));
  }

  void TaskFinish(double time, const ShardTask &task) {
    if (task.GetResourceType() == ResourceType::kCPU) {
      resource_reservation_.SetCPU(resource_reservation_.GetCPU() -
                                   task.GetReq());
    } else if (task.GetResourceType() == ResourceType::kNetwork) {
      resource_reservation_.SetNetwork(resource_reservation_.GetNetwork() -
                                       task.GetReq());
    } else {
      resource_reservation_.SetDisk(resource_reservation_.GetDisk() -
                                    task.GetReq());
    }
    records_.push_back(GenerateResourceReservationRecord(time));
  }

  void SubGraphFinish(double time, double mem) {
    resource_reservation_.SetMemory(resource_reservation_.GetMemory() - mem);
    records_.push_back(GenerateResourceReservationRecord(time));
  }

  void ReportShare(std::ofstream &fout) {
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

private:
  std::vector<double> GenerateResourceReservationRecord(double time) {
    std::vector<double> record;
    record.push_back(time);
    for (int i = 0; i < kNumResourceTypes; ++i) {
      record.push_back(resource_reservation_.GetResourceByIndex(i) /
                       cluster_resource_capacity_.GetResourceByIndex(i));
    }
    return record;
  }

  ResourcePack cluster_resource_capacity_;
  ResourcePack resource_reservation_;
  std::vector<int> jobs_id_;
  std::vector<std::vector<double>> records_;
};

} // namespace simulation
} // namespace axe
