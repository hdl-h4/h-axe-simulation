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
  Event(int eventType, int time, int priority, int job_id, int task_id,
        int work_id)
      : eventType(eventType), time(time), priority(priority), job_id(job_id),
        task_id(task_id), work_id(work_id) {}

  int getEventType() const { return eventType; }
  int getTime() const { return time; }
  void setTime(int time) { time = time; }
  int getPriority() const { return priority; }
  void setPriority(int priority) { priority = priority; }
  int getJobId() const { return job_id; }
  void setJobId(int job_id) { job_id = job_id; }
  int getTaskId() const { return task_id; }
  void setTaskId(int task_id) { task_id = task_id; }
  int getWorkId() const { return work_id; }

  bool operator<(const Event &rhs) const {
    if (time < rhs.getTime())
      return true;
    else if (time > rhs.getTime())
      return false;
    else {
      if (eventType < rhs.getEventType())
        return true;
      else if (eventType > rhs.getEventType())
        return false;
      else
        return priority < rhs.priority;
    }
  }

private:
  int eventType;
  int time;
  int priority;
  int job_id;
  int task_id;
  int work_id;
};

} // namespace simulation
} // namespace axe
