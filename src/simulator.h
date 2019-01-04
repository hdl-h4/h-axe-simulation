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

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <string>

#include "event.h"
#include "scheduler.h"
#include "job_manager.h"

namespace axe {
namespace simulation {

class Simulator {
public:
  void Init() {
    // TODO(SXD): read the configuration file and new JMs and Scheduler
    // read the configuration file and parse into WS and JS
    std::string workers_desc;
    std::vector<std::string> jobs_desc;
    auto scheduler = std::make_shared<Scheduler> (workers_desc);
    auto jms = std::vector<std::shared_ptr<JobManager>>();
    for(const auto& job_desc : jobs_desc) {
      jms.push_back(std::make_shared<JobManager>(job_desc));
    }
  }

  void Serve() {
    //TODO(SXD): process the event in pq one by one according to the priority order
  }

  void Dispatch(Event event) {

  }

private:
  std::map<int, std::function<void(Event event)>> handler_map_;
  std::priority_queue<Event> pq_;
};

} // namespace simulation
} // namespace axe
