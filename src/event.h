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

#include <stdint.h>

namespace axe {
namespace simulation {

enum EventType { TASK_FINISH = 0, NEW_TASK, NEW_JOB };

/**
 * It is actually not elegant design because
 * you don not always need work id or task id.
 * I will try to optimize it later.
 */

class Event {
public:
  Event(int event_type, int time, int priority, int job_id, int task_id,
        int work_id)
      : event_type_(event_type), time_(time), priority_(priority), job_id_(job_id),
        task_id_(task_id), work_id_(work_id) {}

  int getEventType() const { return event_type_; }
  int getTime() const { return time_; }
  void setTime(int time) { time_ = time; }
  int getPriority() const { return priority_; }
  void setPriority(int priority) { priority_ = priority; }
  int getJobId() const { return job_id_; }
  void setJobId(int job_id) { job_id_ = job_id; }
  int getTaskId() const { return task_id_; }
  void setTaskId(int task_id) { task_id_ = task_id; }
  int getWorkId() const { return work_id_; }

  bool operator<(const Event &rhs) const {
    if (time_ < rhs.getTime())
      return true;
    else if (time_ > rhs.getTime())
      return false;
    else {
      if (event_type_ < rhs.getEventType())
        return true;
      else if (event_type_ > rhs.getEventType())
        return false;
      else
        return priority_ < rhs.getPriority();
    }
  }

private:
  int event_type_;
  int time_;
  int priority_;
  int job_id_;
  int task_id_;
  int work_id_;
};

} // namespace simulation
} // namespace axe
