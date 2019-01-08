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

#include "job.h"
#include <memory>
#include <stdint.h>

namespace axe {
namespace simulation {

enum EventType {
  TASK_REQ_FINISH = 0, // SCH tells JM the task req is finished, for JM
  RESOURCE_AVAILABLE,  // SCH tells himself there is resource avaliable after a
                       // task finished, for SCH
  JOB_FINISH,          // JM tells SCH the job is finished, for SCH
  NEW_TASK_REQ,        // JM sends the task req to SCH, for SCH
  NEW_JOB,             // JM sends SCH the new job, for SCH
  RUN_TASK_REQ,        // worker begins to run the task req, for SCH
  JOB_ADMISSION        // SCH accepts the job, for JM
};

const int SCHEDULER = -1;

/**
 * It is actually not elegant design because
 * you don not always need work id or task id.
 * I will try to optimize it later.
 */

class Event {
public:
  Event(int event_type, double time, int priority, int event_principal)
      : event_type_(event_type), time_(time), priority_(priority),
        event_principal_(event_principal) {}

  int GetEventType() const { return event_type_; }
  int GetEventPrincipal() const { return event_principal_; }
  double GetTime() const { return time_; }
  void SetTime(double time) { time_ = time; }
  int GetPriority() const { return priority_; }
  void SetPriority(int priority) { priority_ = priority; }

  bool operator<(const Event &rhs) const {
    if (time_ < rhs.GetTime())
      return true;
    else if (time_ > rhs.GetTime())
      return false;
    else {
      if (event_type_ < rhs.GetEventType())
        return true;
      else if (event_type_ > rhs.GetEventType())
        return false;
      else
        return priority_ < rhs.GetPriority();
    }
  }

private:
  int event_type_;
  int event_principal_;
  double time_;
  int priority_;
};

class NewJobEvent : public Event {
public:
  NewJobEvent(int event_type, double time, int priority, int event_principal,
              const Job &job)
      : Event(event_type, time, priority, event_principal), job_(job) {}

  inline auto &GetJob() const { return job_; }

private:
  Job job_;
};

} // namespace simulation
} // namespace axe
