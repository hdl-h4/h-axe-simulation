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

#include "event.h"
#include "event_handler.h"
#include "event_queue.h"
#include "worker.h"

#include "nlohmann/json.hpp"

namespace axe {
namespace simulation {

using nlohmann::json;

class Scheduler : public EventHandler {

public:
  Scheduler() { RegisterHandler(); }

  inline auto &GetWorkers() const { return workers_; }

  void RegisterHandler() {
    handler_map_.insert({NEW_JOB,
                         [=](const std::shared_ptr<Event> event)
                             -> std::vector<std::shared_ptr<Event>> {
                           // Job admission control
                           std::vector<std::shared_ptr<Event>> event_vector;
                           return event_vector;
                         }});
    handler_map_.insert({JOB_FINISH,
                         [=](const std::shared_ptr<Event> event)
                             -> std::vector<std::shared_ptr<Event>> {
                           std::vector<std::shared_ptr<Event>> event_vector;
                           return event_vector;
                         }});
    handler_map_.insert({NEW_TASK_REQ,
                         [=](const std::shared_ptr<Event> event)
                             -> std::vector<std::shared_ptr<Event>> {
                           std::vector<std::shared_ptr<Event>> event_vector;
                           return event_vector;
                         }});
  }

  std::vector<std::shared_ptr<Event>>
  Handle(const std::shared_ptr<Event> event) {
    // TODO(SXD): handle function for Scheduler
    return handler_map_[event->GetEventType()](event);
  }

  friend void from_json(const json &j, Scheduler &sim) {
    j.at("worker").get_to(sim.workers_);
  }

private:
  std::vector<Worker> workers_;
  std::map<int, std::function<std::vector<std::shared_ptr<Event>>(
                    const std::shared_ptr<Event> event)>>
      handler_map_;
};

} // namespace simulation
} // namespace axe
