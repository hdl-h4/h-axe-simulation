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
#include <string>
#include <vector>

#include "event.h"
#include "job.h"
#include "job_manager.h"
#include "scheduler.h"
#include "worker.h"

namespace axe {
namespace simulation {

using nlohmann::json;

class Simulator {
public:
  Simulator() = default;
  explicit Simulator(const json &j) { from_json(j, *this); }

  void Init(const json &j) {
    // TODO(SXD): read the configuration file and new JMs and Scheduler
    // read the configuration file and parse into WS and JS

    scheduler_ = std::make_shared<Scheduler>(j);
    for (const auto &job : jobs_) {
      jms_.push_back(std::make_shared<JobManager>(job));
    }
    for (const auto &jm : jms_) {
      event_queue.Push(std::static_pointer_cast<Event>(
          std::make_shared<NewJobEvent>(NEW_JOB,
                                        jm->GetJob().GetSubmissionTime(), 0,
                                        SCHEDULER, jm->GetJob())));
    }
  }

  void Serve() {
    // TODO(SXD): process the event in pq one by one according to the priority
    // order
    while (!event_queue.Empty()) {
      auto event = event_queue.Top();
      global_clock = event->GetTime();
      event_queue.Pop();
      event_queue.Push(Dispatch(event));
    }
    std::cout << "simulator server over." << std::endl;
  }

  std::vector<std::shared_ptr<Event>>
  Dispatch(const std::shared_ptr<Event> event) {
    // TODO(SXD): send the event to different components to handle
    int event_principal = event->GetEventPrincipal();
    if (event_principal == SCHEDULER) {
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
    j.at("job").get_to(sim.jobs_);
  }

private:
  std::vector<Job> jobs_;
  std::shared_ptr<Scheduler> scheduler_;
  std::vector<std::shared_ptr<JobManager>> jms_;
};

} // namespace simulation
} // namespace axe
