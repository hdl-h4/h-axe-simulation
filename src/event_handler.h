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

#include <memory>
#include <vector>

#include "event.h"
#include "worker.h"

namespace axe {
namespace simulation {

class EventHandler {
public:
  std::vector<std::shared_ptr<Event>>
  Handle(const std::shared_ptr<Event> event) {
    return handler_map_[event->GetEventType()](event);
  }

  void RegisterHandler(int handler_id,
                       std::function<std::vector<std::shared_ptr<Event>>(
                           const std::shared_ptr<Event> event)>
                           handler) {
    handler_map_.insert({handler_id, handler});
  }

protected:
  std::map<int, std::function<std::vector<std::shared_ptr<Event>>(
                    const std::shared_ptr<Event> event)>>
      handler_map_;
};

} // namespace simulation
} // namespace axe
