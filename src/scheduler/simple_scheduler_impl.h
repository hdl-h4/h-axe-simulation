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

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "algorithm/algorithm_book.h"
#include "event/event.h"
#include "event/job_admission_event.h"
#include "event/job_finish_event.h"
#include "event/new_job_event.h"
#include "event/new_task_req_event.h"
#include "event/placement_decision_event.h"
#include "event_handler.h"
#include "event_queue.h"
#include "scheduler/scheduler_impl.h"
#include "user.h"
#include "worker/worker.h"

namespace axe {
namespace simulation {

class SimpleSchedulerImpl : public SchedulerImpl {
public:
  SimpleSchedulerImpl(std::vector<std::shared_ptr<WorkerAbstract>> &workers,
                      const std::shared_ptr<std::vector<User>> users)
      : SchedulerImpl(workers, users) {}
  std::vector<std::shared_ptr<Event>>
  HandleNewJobEvent(std::shared_ptr<Event> event) override {
    double time = event->GetTime();
    std::vector<std::shared_ptr<Event>> event_vector;
    auto new_job_event = std::static_pointer_cast<NewJobEvent>(event);
    DLOG(INFO) << "job id: " << new_job_event->GetJob().GetJobID();
    event_vector.push_back(std::make_shared<JobAdmissionEvent>(
        JobAdmissionEvent(EventType::JOB_ADMISSION, time, 0,
                          new_job_event->GetJob().GetJobID(),
                          new_job_event->GetJob().GetJobID())));
    return event_vector;
  }
  std::vector<std::shared_ptr<Event>>
  HandleNewReqEvent(std::shared_ptr<Event> event) override {
    // Assign the task on workers
    double time = event->GetTime();
    std::vector<std::shared_ptr<Event>> event_vector;
    std::shared_ptr<NewTaskReqEvent> new_task_req_event =
        std::static_pointer_cast<NewTaskReqEvent>(event);
    DLOG(INFO) << "job id: " << new_task_req_event->GetReq().GetJobID()
               << " subgraph id: "
               << new_task_req_event->GetReq().GetSubGraphID();
    req_queue_.insert(
        {new_task_req_event->GetTime(), new_task_req_event->GetReq()});
    std::vector<std::pair<int, ResourceRequest>> decision_vector =
        AssignReqToWorker();
    for (const auto &decision : decision_vector) {
      DLOG(INFO) << " decision: job id " << decision.second.GetJobID()
                 << ", subgraph id " << decision.second.GetSubGraphID()
                 << ", worker id " << decision.first;
      event_vector.push_back(std::make_shared<PlacementDecisionEvent>(
          PlacementDecisionEvent(EventType::PLACEMENT_DECISION, time, 0,
                                 decision.second.GetJobID(), decision.first,
                                 decision.second.GetSubGraphID())));
    }
    return event_vector;
  }
  std::vector<std::shared_ptr<Event>>
  HandleResourceAvailableEvent(std::shared_ptr<Event> event) override {
    double time = event->GetTime();
    std::vector<std::shared_ptr<Event>> event_vector;
    if (req_queue_.size() == 0)
      return event_vector;
    std::vector<std::pair<int, ResourceRequest>> decision_vector =
        AssignReqToWorker();
    for (const auto &decision : decision_vector) {
      DLOG(INFO) << " decision: job id " << decision.second.GetJobID()
                 << ", subgraph id " << decision.second.GetSubGraphID()
                 << ", worker id " << decision.first;
      event_vector.push_back(std::make_shared<PlacementDecisionEvent>(
          PlacementDecisionEvent(EventType::PLACEMENT_DECISION, time, 0,
                                 decision.second.GetJobID(), decision.first,
                                 decision.second.GetSubGraphID())));
    }
    return event_vector;
  }
  std::vector<std::shared_ptr<Event>>
  HandleJobFinishEvent(std::shared_ptr<Event> event) override {
    std::vector<std::shared_ptr<Event>> event_vector;
    auto job_finish_event = std::dynamic_pointer_cast<JobFinishEvent>(event);
    double sub_time = job_finish_event->GetSubmissionTime();
    double finish_time = job_finish_event->GetTime();
    double use_time = finish_time - sub_time;
    records_.push_back(
        Record{job_finish_event->GetJobID(), sub_time, finish_time, use_time});
    return event_vector;
  }

  std::vector<std::pair<int, ResourceRequest>> AssignReqToWorker() override {
    return AlgorithmBook::GetTaskPlacement()(req_queue_, workers_);
  }

private:
  std::multimap<double, ResourceRequest> req_queue_;
};

} // namespace simulation
} // namespace axe
