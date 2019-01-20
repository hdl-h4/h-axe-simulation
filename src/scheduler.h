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

#include "algorithm/algorithm_book.h"
#include "event/event.h"
#include "event/job_admission_event.h"
#include "event/job_finish_event.h"
#include "event/new_job_event.h"
#include "event/new_task_req_event.h"
#include "event/placement_decision_event.h"
#include "event_handler.h"
#include "event_queue.h"
#include "user.h"
#include "worker.h"

#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace axe {
namespace simulation {

using nlohmann::json;

class Scheduler : public EventHandler {
public:
  Scheduler(const std::shared_ptr<std::vector<Worker>> workers,
            const std::shared_ptr<std::vector<User>> users)
      : workers_(workers), users_(users) {
    RegisterHandlers();
  }

  void Report() {
    std::string file_name = "report/jobs_report";
    std::ofstream fout(file_name, std::ios::out);
    fout << "job id" << std::setw(20) << "submission time" << std::setw(20)
         << "finish time" << std::setw(20) << "use time" << std::setw(5)
         << std::endl;
    for (const auto &record : records_) {
      fout << record.job_id << std::setw(20) << record.submission_time
           << std::setw(20) << record.finish_time << std::setw(20)
           << record.use_time << std::setw(5) << std::endl;
    }
    fout.close();
  }

  void RegisterHandlers() {
    RegisterHandler(
        NEW_JOB,
        [&](std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          double time = event->GetTime();
          std::vector<std::shared_ptr<Event>> event_vector;
          std::shared_ptr<NewJobEvent> new_job_event =
              std::static_pointer_cast<NewJobEvent>(event);
          DLOG(INFO) << "job id: " << new_job_event->GetJob().GetJobId();
          event_vector.push_back(std::make_shared<JobAdmissionEvent>(
              JobAdmissionEvent(JOB_ADMISSION, time, 0,
                                new_job_event->GetJob().GetJobId(),
                                new_job_event->GetJob().GetJobId())));

          return event_vector;
        });
    RegisterHandler(
        NEW_TASK_REQ,
        [&](std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
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
            event_vector.push_back(
                std::make_shared<PlacementDecisionEvent>(PlacementDecisionEvent(
                    PLACEMENT_DECISION, time, 0, decision.second.GetJobID(),
                    decision.first, decision.second.GetSubGraphID())));
          }
          return event_vector;
        });
    RegisterHandler(
        RESOURCE_AVAILABLE,
        [&](std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
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
            event_vector.push_back(
                std::make_shared<PlacementDecisionEvent>(PlacementDecisionEvent(
                    PLACEMENT_DECISION, time, 0, decision.second.GetJobID(),
                    decision.first, decision.second.GetSubGraphID())));
          }
          return event_vector;
        });
    RegisterHandler(JOB_FINISH,
                    [&](std::shared_ptr<Event> event)
                        -> std::vector<std::shared_ptr<Event>> {
                      std::vector<std::shared_ptr<Event>> event_vector;
                      std::shared_ptr<JobFinishEvent> job_finish_event =
                          std::static_pointer_cast<JobFinishEvent>(event);
                      double sub_time = job_finish_event->GetSubmissionTime();
                      double finish_time = job_finish_event->GetTime();
                      double use_time = finish_time - sub_time;
                      records_.push_back(Record{job_finish_event->GetJobId(),
                                                sub_time, finish_time,
                                                use_time});
                      return event_vector;
                    });
  }

  std::vector<std::pair<int, ResourceRequest>> AssignReqToWorker() {
    return AlgorithmBook::GetTaskPlacement()(req_queue_, workers_);
  }

  struct Record {
    int job_id;
    double submission_time;
    double finish_time;
    double use_time;
  };

private:
  std::vector<Record> records_;
  std::multimap<double, ResourceRequest> req_queue_;
  std::shared_ptr<std::vector<Worker>> workers_;
  std::shared_ptr<std::vector<User>> users_;
};

} // namespace simulation
} // namespace axe
