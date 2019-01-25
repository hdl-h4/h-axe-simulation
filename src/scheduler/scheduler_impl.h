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
#include "user.h"
#include "worker/worker.h"

namespace axe {
namespace simulation {

class SchedulerImpl {
public:
  static std::shared_ptr<SchedulerImpl>
  CreateImpl(const std::shared_ptr<std::vector<Worker>> &workers,
             const std::shared_ptr<std::vector<User>> &users,
             const std::string &mode);
  struct Record {
    int job_id;
    double submission_time;
    double finish_time;
    double use_time;
  };

  SchedulerImpl(const std::shared_ptr<std::vector<Worker>> &workers,
                const std::shared_ptr<std::vector<User>> &users)
      : workers_(workers), users_(users) {}
  virtual std::vector<std::shared_ptr<Event>>
  HandleNewJobEvent(std::shared_ptr<Event> event) = 0;
  virtual std::vector<std::shared_ptr<Event>>
  HandleNewReqEvent(std::shared_ptr<Event> event) = 0;
  virtual std::vector<std::shared_ptr<Event>>
  HandleResourceAvailableEvent(std::shared_ptr<Event> event) = 0;
  virtual std::vector<std::shared_ptr<Event>>
  HandleJobFinishEvent(std::shared_ptr<Event> event) = 0;

  void Report() {
    std::string file_name = "report/jobs_report";
    std::ofstream fout(file_name, std::ios::out);
    fout << "job id" << std::setw(20) << "submission time" << std::setw(20)
         << "finish time" << std::setw(20) << "job completion time"
         << std::setw(5) << std::endl;
    for (const auto &record : records_) {
      fout << record.job_id << std::setw(20) << record.submission_time
           << std::setw(20) << record.finish_time << std::setw(20)
           << record.use_time << std::setw(5) << std::endl;
    }
    fout.close();
  }

  virtual std::vector<std::pair<int, ResourceRequest>> AssignReqToWorker() = 0;

  // TODO (czk) Do scheduling in a time interval

protected:
  std::shared_ptr<std::vector<Worker>> workers_;
  std::vector<Record> records_;
  std::shared_ptr<std::vector<User>> users_;
};

} // namespace simulation
} // namespace axe
