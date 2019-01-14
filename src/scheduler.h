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

#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include "algorithm/algorithm_book.h"
#include "event/event.h"
#include "event/new_job_event.h"
#include "event/new_task_req_event.h"
#include "event_handler.h"
#include "event_queue.h"
#include "worker.h"

namespace axe {
namespace simulation {

using nlohmann::json;

class Scheduler : public EventHandler {
public:
  Scheduler() { RegisterHandlers(); }

  void RegisterHandlers() {
    RegisterHandler(NEW_JOB, [&](std::shared_ptr<Event> event)
                                 -> std::vector<std::shared_ptr<Event>> {
      std::vector<std::shared_ptr<Event>> event_vector;
      std::shared_ptr<NewJobEvent> new_job_event =
          std::static_pointer_cast<NewJobEvent>(event);
      DLOG(INFO) << "job id: " << new_job_event->GetJob().GetId();
      event_vector.push_back(std::make_shared<JobAdmissionEvent>(
          JobAdmissionEvent(JOB_ADMISSION, global_clock, 0,
                            new_job_event->GetJob().GetId(),
                            new_job_event->GetJob().GetId())));

      return event_vector;
    });
    RegisterHandler(NEW_TASK_REQ, [&](std::shared_ptr<Event> event)
                                      -> std::vector<std::shared_ptr<Event>> {
      // Assign the task on workers
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
        event_vector.push_back(std::make_shared<PlacementDecisionEvent>(
            PlacementDecisionEvent(PLACEMENT_DECISION, global_clock, 0,
                                   decision.second.GetJobID(), decision.first,
                                   decision.second.GetSubGraphID())));
      }
      return event_vector;
    });
    RegisterHandler(
        RESOURCE_AVAILABLE, [&](std::shared_ptr<Event> event)
                                -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;
          if (req_queue_.size() == 0)
            return event_vector;
          std::vector<std::pair<int, ResourceRequest>> decision_vector =
              AssignReqToWorker();
          for (const auto &decision : decision_vector) {
            event_vector.push_back(std::make_shared<PlacementDecisionEvent>(
                PlacementDecisionEvent(PLACEMENT_DECISION, global_clock, 0,
                                       decision.second.GetJobID(),
                                       decision.first,
                                       decision.second.GetSubGraphID())));
          }
          return event_vector;
        });
    RegisterHandler(JOB_FINISH, [&](std::shared_ptr<Event> event)
                                    -> std::vector<std::shared_ptr<Event>> {
      std::vector<std::shared_ptr<Event>> event_vector;
      return event_vector;
    });
  }

  std::vector<std::pair<int, ResourceRequest>> AssignReqToWorker() {
    return AlgorithmBook::GetTaskPlacement()(req_queue_);
  }

private:
  std::multimap<double, ResourceRequest> req_queue_;
};

} // namespace simulation
} // namespace axe
