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
#include "worker/worker.h"

namespace axe {
namespace simulation {

using nlohmann::json;

class Simulator {
public:
  Simulator() = default;
  explicit Simulator(const json &workers_json, const json &jobs_json) {
    invalid_event_id_set_ = std::make_shared<std::set<int>>();
    workers_ = std::make_shared<std::vector<Worker>>();
    users_ = std::make_shared<std::vector<User>>();
    from_json(workers_json, *this);
    from_json(jobs_json, *this);
    for (auto &worker : *workers_) {
      worker.Init(invalid_event_id_set_);
    }
  }

  void Init() {
    // TODO(SXD): read the configuration file and new JMs and Scheduler
    // read the configuration file and parse into WS and JS

    scheduler_ = std::make_shared<Scheduler>(workers_, users_);

    for (const auto &worker : *workers_) {
      cluster_resource_capacity_.AddToMe(worker.GetRemainResourcePack());
    }

    int size = jobs_.size();
    int users_size = 0;
    for (const auto &job : jobs_) {
      jms_.push_back(std::make_shared<JobManager>(job, workers_, users_));
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
    std::string prefix = "report/worker_";
    std::string suffix = ".csv";
    for (int i = 0; i < workers_->size(); ++i) {
      std::ofstream fout(prefix + std::to_string(i) + suffix, std::ios::out);
      (*workers_)[i].ReportUtilization(fout);
      fout.close();
    }
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
      j.at("worker").get_to(*(sim.workers_));
    } else if (j.find("job") != j.end()) {
      j.at("job").get_to(sim.jobs_);
    }
  }

private:
  ResourcePack cluster_resource_capacity_;
  std::vector<Job> jobs_;
  std::shared_ptr<Scheduler> scheduler_;
  std::vector<std::shared_ptr<JobManager>> jms_;
  EventQueue event_queue_;
  std::shared_ptr<std::vector<Worker>> workers_;
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
