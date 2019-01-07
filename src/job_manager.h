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
#include "event_handler.h"
#include "event_queue.h"
#include "job.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace axe {
namespace simulation {

class JobManager : public EventHandler {
public:
  JobManager(const Job &job) : job_(job) { RegisterHandler(); }

  void RegisterHandler() {
    handler_map_.insert(
        {TASK_REQ_FINISH, [=](const std::shared_ptr<Event> event) {

         }});
  }

  void Handle(const std::shared_ptr<Event> event) {
    // TODO(SXD): handle function for JM
    handler_map_[event->GetEventType()](event);
  }

  inline auto &GetJob() const { return job_; }

  void BuildDependencies() {
    for (auto &task : job_.GetTasks()) {
      for (auto child : task.GetChildren()) {
        if (child.second == Dependency::Sync) {
          dep_counter_[child.first] += task.GetParallelism();
        } else {
          ++dep_counter_[child.first];
        }
      }
    }
  }

private:
  std::map<int, std::function<void(const std::shared_ptr<Event> event)>>
      handler_map_;
  Job job_;
  std::unordered_map<int, std::vector<Task>> id_to_sharded_task_;
  std::unordered_map<int, int> dep_counter_;
  std::map<std::pair<int, int>, int> dep_finish_counter_;
};

} // namespace simulation
} // namespace axe
