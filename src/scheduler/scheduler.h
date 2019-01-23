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

#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include "scheduler/scheduler_impl.h"

namespace axe {
namespace simulation {

using nlohmann::json;

class Scheduler : public EventHandler {
public:
  Scheduler(const std::shared_ptr<std::vector<Worker>> workers,
            const std::shared_ptr<std::vector<User>> users) {
    // TODO(czk) Change Mode
    impl_ = SchedulerImpl::CreateImpl(workers, users, "RealTime");
    RegisterHandlers();
  }

  void RegisterHandlers() {
    RegisterHandler(NEW_JOB, [&](std::shared_ptr<Event> event)
                                 -> std::vector<std::shared_ptr<Event>> {
      return impl_->HandleNewJobEvent(event);
    });
    RegisterHandler(NEW_TASK_REQ, [&](std::shared_ptr<Event> event)
                                      -> std::vector<std::shared_ptr<Event>> {
      return impl_->HandleNewReqEvent(event);
    });
    RegisterHandler(RESOURCE_AVAILABLE,
                    [&](std::shared_ptr<Event> event)
                        -> std::vector<std::shared_ptr<Event>> {
                      return impl_->HandleResourceAvailableEvent(event);
                    });
    RegisterHandler(JOB_FINISH, [&](std::shared_ptr<Event> event)
                                    -> std::vector<std::shared_ptr<Event>> {
      return impl_->HandleJobFinishEvent(event);
    });
  }

  void Report() { impl_->Report(); }

private:
  std::shared_ptr<SchedulerImpl> impl_;
};

} // namespace simulation
} // namespace axe
