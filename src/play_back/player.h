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

#include <set>

#include "common.h"
#include "job_record.h"
#include "resource/resource.h"
#include "unit.h"

namespace axe {
namespace simulation {

struct RecordEvent {
  int event_type;
  int64_t time;
  int worker_id;
  int resource_type;
  double resource_request;
  double memory;
  double duration;
};

struct DurationRecord {
  int shard_id;
  int instance_id;
  double duration;
};

bool UnitComparator(Unit i, Unit j) {
  return i.GetAssignedTime() < j.GetAssignedTime();
}

struct RecordEventCompare {
  bool operator()(const RecordEvent &lhs, const RecordEvent &rhs) {
    return lhs.time > rhs.time;
  }
};

class Player {
public:
  void Init(const json &j) {
    from_json(j, *this);
    CollectUnits();
    FixTimeLine(GetStartPoint());
    worker_num_ = ComputeWorkerNum();
    for (int i = 0; i <= worker_num_; ++i) {

      std::vector<double> tmp = {0, 0, 0, 0};

      worker_snapshot_.insert({i, tmp});
    }
    sort(units_.begin(), units_.end(), UnitComparator);
    // Print();
    GenerateEvent();
    GenerateRecords();
    ReportUtilization();
    CollectDurationRecord();
    PrintDurationRecord();
    GetPeakUsageForEachWork();
  }

  void Print() {
    for (const auto &unit : units_) {

      std::cout << "Assigned Time: " << unit.GetAssignedTime() << std::endl;
      std::cout << "Finished Time: " << unit.GetFinishedTime() << std::endl;
      std::cout << "Locality: " << unit.GetLocality() << std::endl;
      std::cout << "Resource Type: " << unit.GetResourceType() << std::endl;
      std::cout << "Memory: " << unit.GetMemory() << std::endl;
      std::cout << "Worker ID: " << unit.GetWorkerID() << std::endl;
      std::cout << "Resource Request: " << unit.GetResourceRequest()
                << std::endl;
    }
  }

  void PrintDurationRecord() {
    for (int i = 0; i < unit_duration_record_.size(); ++i) {
      std::cout << "task " << i << std::endl;
      std::cout << "mean: " << statistic_record_[i][0] << std::endl;
      std::cout << "variance: " << statistic_record_[i][1] << std::endl;
      std::cout << "max: " << statistic_record_[i][2] << std::endl;
      std::cout << "min: " << statistic_record_[i][3] << std::endl << std::endl;
    }
  }

  void GetPeakUsageForEachWork() {
    peak_cpu_usage_.resize(worker_num_);
    peak_mem_usage_.resize(worker_num_);
    peak_net_usage_.resize(worker_num_);
    peak_disk_usage_.resize(worker_num_);
    for (int k = 0; k <= worker_num_; ++k) {
      peak_cpu_usage_[k] = peak_mem_usage_[k] = peak_net_usage_[k] =
          peak_disk_usage_[k] = 0;
      for (auto iter = worker_record_.at(k).begin();
           iter != worker_record_.at(k).end(); ++iter) {
        if ((iter->second)[size_t(ResourceType::kNetwork)] > peak_net_usage_[k])
          peak_net_usage_[k] = (iter->second)[size_t(ResourceType::kNetwork)];
        if ((iter->second)[size_t(ResourceType::kDisk)] > peak_disk_usage_[k])
          peak_disk_usage_[k] = (iter->second)[size_t(ResourceType::kDisk)];
        if ((iter->second)[size_t(ResourceType::kMemory)] > peak_mem_usage_[k])
          peak_mem_usage_[k] = (iter->second)[size_t(ResourceType::kMemory)];
        if ((iter->second)[size_t(ResourceType::kCPU)] > peak_cpu_usage_[k])
          peak_cpu_usage_[k] = (iter->second)[size_t(ResourceType::kCPU)];
      }
      std::cout << "--------------------------------" << std::endl;
      std::cout << "worker " << k << " peak usage: " << std::endl;
      std::cout << "cpu peak usage: " << peak_cpu_usage_[k] << std::endl;
      std::cout << "mem peak usage: " << peak_mem_usage_[k] << std::endl;
      std::cout << "disk peak usage: " << peak_disk_usage_[k] << std::endl;
      std::cout << "network peak usage: " << peak_net_usage_[k] << std::endl;
      std::cout << "--------------------------------" << std::endl;
    }
  }

  void CollectDurationRecord() {
    unit_duration_record_.resize(ComputeTaskNum() + 1);
    statistic_record_.resize(ComputeTaskNum() + 1);

    for (auto const &unit : units_) {
      unit_duration_record_[unit.GetTaskID()].push_back(
          (double)(unit.GetFinishedTime() - unit.GetAssignedTime()));
    }

    for (int i = 0; i < unit_duration_record_.size(); ++i) {
      double avg = std::accumulate(unit_duration_record_[i].begin(),
                                   unit_duration_record_[i].end(), 0) /
                   (double)unit_duration_record_[i].size();
      statistic_record_[i].push_back(avg);

      double std_err = 0;
      for (auto const &r : unit_duration_record_[i]) {
        std_err += (r - avg) * (r - avg);
      }
      std_err /= unit_duration_record_[i].size();
      statistic_record_[i].push_back(std_err);

      double max = unit_duration_record_[i][0];
      for (auto const &r : unit_duration_record_[i]) {
        max = (max > r) ? max : r;
      }
      statistic_record_[i].push_back(max);

      double min = unit_duration_record_[i][0];
      for (auto const &r : unit_duration_record_[i]) {
        min = (min < r) ? min : r;
      }
      statistic_record_[i].push_back(min);
    }
  }

  void GenerateEvent() {
    for (auto const &unit : units_) {

      // if(unit.GetWorkerID() == 4 && unit.GetResourceType() == 0) {
      //   std::cout << "worker4 has a cpu: " << unit.GetResourceRequest() <<
      //   std::endl;
      //   std::cout << "start time: " << unit.GetAssignedTime() << " " <<
      //   "finished time: " << unit.GetFinishedTime() << std::endl;
      // }

      RecordEvent start_event = {
          0,
          unit.GetAssignedTime(),
          unit.GetWorkerID(),
          unit.GetResourceType(),
          unit.GetResourceRequest(),
          unit.GetMemory(),
          (double)(unit.GetFinishedTime() - unit.GetAssignedTime() + 1)};
      // if(unit.GetWorkerID() == 2) std::cout << start_event.resource_request
      // << std::endl;
      RecordEvent finish_event = {
          1,
          unit.GetFinishedTime(),
          unit.GetWorkerID(),
          unit.GetResourceType(),
          unit.GetResourceRequest(),
          unit.GetMemory(),
          (double)(unit.GetFinishedTime() - unit.GetAssignedTime() + 1)};
      // if(unit.GetWorkerID() == 2) std::cout << "duration: " <<
      // unit.GetFinishedTime() - unit.GetAssignedTime() << std::endl;
      event_queue_.push(start_event);
      event_queue_.push(finish_event);
    }
  }

  void GenerateRecords() {
    for (int i = 0; i <= worker_num_; ++i) {
      std::map<int64_t, std::vector<double>> each_worker_record;
      each_worker_record.insert({0, {0, 0, 0, 0}});
      worker_record_.insert({i, each_worker_record});
    }

    while (!event_queue_.empty()) {
      RecordEvent event = event_queue_.top();

      std::vector<double> record = GenerateRecord(event);

      // if (event.worker_id == 6) {
      // std::cout << event.resource_request << std::endl;
      // std::cout << event.time << std::endl;
      // std::cout << "--------------------------\n";
      // for (int i = 0; i < record.size(); ++i) {
      // std::cout << record[i] << std::endl;
      // }
      // std::cout << "--------------------------\n";
      // std::cout << std::endl;
      // }
      worker_record_.at(event.worker_id).insert({event.time, record});
      event_queue_.pop();
    }

    // std::cout << "print: " << std::endl;

    // for (auto iter = worker_record_.at(2).begin();
    //  iter != worker_record_.at(2).end(); ++iter) {
    // std::cout << "event time: " << iter->first << std::endl;
    // std::cout << "resource: " << (iter->second)[0] << ' ' <<
    // (iter->second)[1]
    // << ' ' << (iter->second)[2] << ' ' << (iter->second)[3]
    // << std::endl;
    // }
  }

  std::vector<double> GenerateRecord(RecordEvent event) {
    std::vector<double> record;
    int worker_id = event.worker_id;
    // std::cout << "event type: " << event.event_type << std::endl;
    if (event.resource_request <= 0)
      CHECK(false) << "< 0";
    if (event.event_type == 0) {
      worker_snapshot_.at(worker_id)[size_t(ResourceType::kMemory)] +=
          event.memory;
      if (event.resource_type == 0) {
        // CPU
        worker_snapshot_.at(worker_id)[size_t(ResourceType::kCPU)] +=
            event.resource_request;
      } else if (event.resource_type == 1) {
        // Network
        // worker_snapshot_.at(worker_id)[size_t(ResourceType::kNetwork)] +=
        // event.resource_request / event.duration / max_net_usage_;

        worker_snapshot_.at(worker_id)[size_t(ResourceType::kNetwork)] +=
            event.resource_request / event.duration;
      } else {
        // Disk
        // worker_snapshot_.at(worker_id)[size_t(ResourceType::kDisk)] +=
        // event.resource_request / event.duration / max_disk_usage_;

        worker_snapshot_.at(worker_id)[size_t(ResourceType::kCPU)] += 1;
        // std::cout << "disk time: " << event.resource_request /
        // event.duration;
      }
    } else {
      worker_snapshot_.at(worker_id)[size_t(ResourceType::kMemory)] -=
          event.memory;
      if (event.resource_type == 0) {
        // CPU
        worker_snapshot_.at(worker_id)[size_t(ResourceType::kCPU)] -=
            event.resource_request;
      } else if (event.resource_type == 1) {
        // Network
        // worker_snapshot_.at(worker_id)[size_t(ResourceType::kNetwork)] -=
        // event.resource_request / event.duration / max_net_usage_;
        worker_snapshot_.at(worker_id)[size_t(ResourceType::kNetwork)] -=
            event.resource_request / event.duration;
        if (worker_snapshot_.at(worker_id)[size_t(ResourceType::kNetwork)] < 0)
          worker_snapshot_.at(worker_id)[size_t(ResourceType::kNetwork)] = 0;
      } else {
        // Disk
        // worker_snapshot_.at(worker_id)[size_t(ResourceType::kDisk)] -=
        // event.resource_request / event.duration / max_disk_usage_;
        worker_snapshot_.at(worker_id)[size_t(ResourceType::kCPU)] -= 1;
        // std::cout << "disk time: " << event.resource_request /
        // event.duration;
        if (worker_snapshot_.at(worker_id)[size_t(ResourceType::kCPU)] < 0)
          worker_snapshot_.at(worker_id)[size_t(ResourceType::kCPU)] = 0;
      }
    }

    return worker_snapshot_.at(worker_id);
  }

  void ReportUtilization() {
    std::string prefix = "report/player_worker_";
    std::string suffix = ".csv";

    for (int k = 0; k <= worker_num_; ++k) {
      std::ofstream fout(prefix + std::to_string(k) + suffix, std::ios::out);
      fout << "#CPU"
           << "\t"
           << "MEMORY"
           << "\t"
           << "DISK"
           << "\t"
           << "NETWORK" << std::endl;
      for (auto iter = worker_record_.at(k).begin();
           iter != worker_record_.at(k).end(); ++iter) {
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
        if (next_iter != worker_record_.at(k).end()) {
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
      fout.close();
    }
  }

  friend void from_json(const json &j, Player &player) {
    j.get_to(player.job_record_);
  }

private:
  void FixTimeLine(int64_t time) {
    for (auto &unit : units_) {
      unit.SetAssignedTime(unit.GetAssignedTime() - time);
      unit.SetFinishedTime(unit.GetFinishedTime() - time);
    }
  }

  void CollectUnits() {
    auto u = job_record_.GetUnits();
    units_.insert(units_.end(), u.begin(), u.end());
  }

  int64_t GetStartPoint() {
    int64_t start = units_[0].GetAssignedTime();
    for (const auto &unit : units_) {
      start = (start > unit.GetAssignedTime()) ? unit.GetAssignedTime() : start;
    }
    // std::cout << "start point: " << start << std::endl;
    return start;
  }

  int ComputeWorkerNum() {
    int worker_num = units_[0].GetWorkerID();
    for (const auto &unit : units_) {
      worker_num =
          (worker_num > unit.GetWorkerID()) ? worker_num : unit.GetWorkerID();
    }
    return worker_num;
  }

  int ComputeTaskNum() {
    int task_num = units_[0].GetTaskID();
    for (const auto &unit : units_) {
      task_num = (task_num > unit.GetTaskID()) ? task_num : unit.GetTaskID();
    }

    return task_num;
  }

  JobRecord job_record_;
  std::vector<Unit> units_;
  int worker_num_;
  std::map<int, std::vector<double>> worker_snapshot_;
  std::map<int, std::map<int64_t, std::vector<double>>> worker_record_;
  std::priority_queue<RecordEvent, std::vector<RecordEvent>, RecordEventCompare>
      event_queue_;
  double max_net_usage_ = 1;
  double max_disk_usage_ = 1;
  double max_cpu_usage_ = 1;

  std::vector<std::vector<double>> unit_duration_record_;
  std::vector<std::vector<double>> statistic_record_;

  std::vector<double> peak_cpu_usage_;
  std::vector<double> peak_mem_usage_;
  std::vector<double> peak_net_usage_;
  std::vector<double> peak_disk_usage_;
};
}
}
