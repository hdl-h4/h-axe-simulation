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

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "glog/logging.h"

#include "event/event.h"
#include "job/job.h"
#include "job_manager.h"
#include "scheduler/scheduler.h"
#include "user.h"
#include "worker/node_manager.h"
#include "worker/worker.h"

namespace axe {
namespace simulation {

using nlohmann::json;

class Simulator {
public:
  Simulator() = default;
  explicit Simulator(const json &workers_json, const json &jobs_json,
                     const std::string &executor) {
    invalid_event_id_set_ = std::make_shared<std::set<int>>();

    if (executor == "YARN") {
      is_worker_ = false;
    }

    users_ = std::make_shared<std::vector<User>>();
    from_json(workers_json, *this);
    from_json(jobs_json, *this);
    if (is_worker_) {
      for (auto worker : workers_) {
        workers_abstract_.push_back(worker);
      }
    } else {
      for (auto worker : node_manager_) {
        workers_abstract_.push_back(worker);
      }
    }

    for (int i = 0; i < workers_abstract_.size(); ++i) {
      workers_abstract_[i]->Init(i, invalid_event_id_set_);
    }
  }

  void Init(const std::string &executor) {
    // TODO(SXD): read the configuration file and new JMs and Scheduler
    // read the configuration file and parse into WS and JS

    scheduler_ = std::make_shared<Scheduler>(workers_abstract_, users_);

    for (const auto &worker : workers_abstract_) {
      cluster_resource_capacity_.AddToMe(worker->GetRemainResourcePack());
    }

    int size = jobs_.size();
    int users_size = 0;
    for (const auto &job : jobs_) {
      jms_.push_back(std::make_shared<JobManager>(job, workers_abstract_,
                                                  users_, executor));
      users_size = std::max(users_size, job.GetUserID() + 1);
    }

    users_->resize(users_size);
    for (const auto &job : jobs_) {
      users_->at(job.GetUserID()).AddJobID(job.GetJobID());
    }

    for (auto &user : *users_) {
      user.SetClusterResourceCapacity(cluster_resource_capacity_);
    }

    for (const auto &jm : jms_) {
      event_queue_.Push(std::make_shared<NewJobEvent>(
          EventType::NEW_JOB, jm->GetJob().GetSubmissionTime(), 0, kScheduler,
          jm->GetJob()));
    }
  }

  void Serve() {
    // TODO(SXD): process the event in pq one by one according to the priority
    // order
    while (!event_queue_.Empty()) {
      auto event = event_queue_.Top();
      DLOG(INFO) << "event id: " << event->GetEventID()
                 << " event type: " << event_map_[event->GetEventType()];
      event_queue_.Pop();
      // if the event is invalid, then skip it
      auto it = invalid_event_id_set_->find(event->GetEventID());
      if (it == invalid_event_id_set_->end()) {
        event_queue_.Push(Dispatch(event));
      } else {
        invalid_event_id_set_->erase(it);
        DLOG(INFO) << "skip invalid event: " << event->GetEventID();
      }
    }
  }

  void Report() {
    average_records_.resize(100000);
    for (int i = 0; i < 100000; i++) {
      average_records_.at(i).resize(4);
    }
    std::string prefix = "report/worker_";
    std::string suffix = ".csv";
    for (int i = 0; i < workers_abstract_.size(); ++i) {
      std::ofstream fout(prefix + std::to_string(i) + suffix, std::ios::out);
      workers_abstract_[i]->ReportUtilization(fout, average_records_);
      fout.close();
      std::cout << "average usage of worker " << i
                << "   CPU : " << workers_abstract_[i]->GetAverageUsage()[0]
                << ",  Memory : " << workers_abstract_[i]->GetAverageUsage()[1]
                << ",  Disk : " << workers_abstract_[i]->GetAverageUsage()[2]
                << ",  Network : " << workers_abstract_[i]->GetAverageUsage()[3]
                << "\n";
    }
    std::ofstream fout(std::string("report/cluster_average") + suffix,
                       std::ios::out);
    fout << "#CPU"
         << "\t"
         << "MEMORY"
         << "\t"
         << "DISK"
         << "\t"
         << "NETWORK" << std::endl;

    int worker_num = workers_abstract_.size();
    std::vector<double> average_;
    average_.resize(4);
    for (int i = 0; i < 100000; i++) {
      for (int j = 0; j < average_records_.at(i).size(); j++) {
        average_[j] += average_records_[i][j] / worker_num;
        fout << (average_records_[i][j] / worker_num);
        if (j < average_records_.at(i).size() - 1)
          fout << "\t";
      }
      fout << std::endl;
    }
    fout.close();
    std::cout << "average usage of cluster "
              << "   CPU : " << average_[0] / 100000
              << ",  Memory : " << average_[1] / 100000
              << ",  Disk : " << average_[2] / 100000
              << ",  Network : " << average_[3] / 100000 << "\n";
    scheduler_->Report();
    prefix = "report/user_";
    for (int i = 0; i < users_->size(); ++i) {
      std::ofstream fout(prefix + std::to_string(i), std::ios::out);
      (*users_)[i].ReportShare(fout);
      fout.close();
    }
  }

  std::vector<std::shared_ptr<Event>> Dispatch(std::shared_ptr<Event> event) {
    // TODO(SXD): send the event to different components to handle
    int event_principal = event->GetEventPrincipal();
    if (event_principal == kScheduler) {
      return scheduler_->Handle(event);
    } else {
      return jms_[event_principal]->Handle(event);
    }
  }

  // for debug
  void Print() {
    for (auto &job : jobs_) {
      job.Print();
    }
  }

  friend void from_json(const json &j, Simulator &sim) {
    if (j.find("worker") != j.end()) {
      if (sim.is_worker_) {
        j.at("worker").get_to(sim.workers_);
      } else {
        j.at("worker").get_to(sim.node_manager_);
      }
    } else if (j.find("job") != j.end()) {
      j.at("job").get_to(sim.jobs_);
    }
  }

private:
  bool is_worker_ = true;
  std::vector<std::vector<double>> average_records_;
  ResourcePack cluster_resource_capacity_;
  std::vector<Job> jobs_;
  std::shared_ptr<Scheduler> scheduler_;
  std::vector<std::shared_ptr<JobManager>> jms_;
  EventQueue event_queue_;
  std::vector<std::shared_ptr<Worker>> workers_;
  std::vector<std::shared_ptr<NodeManager>> node_manager_;
  std::vector<std::shared_ptr<WorkerAbstract>> workers_abstract_;
  std::shared_ptr<std::vector<User>> users_;
  std::map<EventType, std::string> event_map_ = {
      {EventType::TASK_FINISH, "TASK FINISH"},
      {EventType::RESOURCE_AVAILABLE, "RESOURCE AVAILABLE"},
      {EventType::JOB_FINISH, "JOB FINISH"},
      {EventType::JOB_ADMISSION, "JOB ADMISSION"},
      {EventType::PLACEMENT_DECISION, "PLACEMENT DECISION"},
      {EventType::NEW_TASK_REQ, "NEW TASK REQ"},
      {EventType::NEW_JOB, "NEW JOB"}};
  std::shared_ptr<std::set<int>> invalid_event_id_set_;
};

} // namespace simulation
} // namespace axe
