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
#include "shard_task.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace axe {
namespace simulation {

class JobManager : public EventHandler {
public:
  JobManager(const Job &job) : job_(job) {
    RegisterHandler();
    BuildDependencies();
  }

  void RegisterHandler() {
    handler_map_.insert({TASK_REQ_FINISH,
                         [=](const std::shared_ptr<Event> event)
                             -> std::vector<std::shared_ptr<Event>> {
                           std::vector<std::shared_ptr<Event>> event_vector;
                           return event_vector;
                         }});
  }

  std::vector<std::shared_ptr<Event>>
  Handle(const std::shared_ptr<Event> event) {
    // TODO(SXD): handle function for JM
    return handler_map_[event->GetEventType()](event);
  }

  inline auto &GetJob() const { return job_; }

  void BuildDependencies() {
    for (int i = 0; i < job_.GetSubGraphs().size(); ++i) {
      auto &sg = job_.GetSubGraphs().at(i);
      for (auto &st : sg.GetShardTasks()) {
        shard_task_to_subgraph_[std::make_pair(st.GetTaskId(),
                                               st.GetShardId())] = i;
        for (auto &child : st.GetChildren()) {
          dep_counter_[child]++;
        }
      }
    }
  }

private:
  std::map<int, std::function<std::vector<std::shared_ptr<Event>>(
                    const std::shared_ptr<Event> event)>>
      handler_map_;
  Job job_;
  std::map<std::pair<int, int>, int> shard_task_to_subgraph_;
  std::map<std::pair<int, int>, int> dep_counter_;
  std::map<std::pair<int, int>, int> dep_finish_counter_;
};

} // namespace simulation
} // namespace axe
