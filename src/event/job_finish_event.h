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

#include "event.h"
#include "job/job.h"

namespace axe {
namespace simulation {

class JobFinishEvent : public Event {
public:
  JobFinishEvent(EventType event_type, double time, int priority,
                 int event_principal, int job_id, double submission_time)
      : Event(event_type, time, priority, event_principal), job_id_(job_id),
        submission_time_(submission_time) {}

  inline int GetJobID() const { return job_id_; }
  inline double GetSubmissionTime() const { return submission_time_; }

private:
  int job_id_;
  double submission_time_;
};

} // namespace simulation
} // namespace axe
