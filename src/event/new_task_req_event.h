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
#include "resource/resource_request.h"

namespace axe {
namespace simulation {

class NewTaskReqEvent : public Event {
public:
  NewTaskReqEvent(int event_type, double time, int priority,
                  int event_principal, const ResourceRequest &req)
      : Event(event_type, time, priority, event_principal), req_(req) {}

  inline auto &GetReq() const { return req_; }

private:
  ResourceRequest req_;
};

} // namespace simulation
} // namespace axe
