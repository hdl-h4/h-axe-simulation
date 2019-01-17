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

#include "task_placement.h"
#include "glog/logging.h"
#include "resource/resource_request.h"
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <vector>

namespace axe {
namespace simulation {

std::vector<std::pair<int, ResourceRequest>>
FIFO(std::multimap<double, ResourceRequest> &req_queue,
     std::shared_ptr<std::vector<Worker>> workers) {
  DLOG(INFO) << "task placement: FIFO";
  static int worker_id = -1;
  worker_id = (worker_id + 1) % (*workers).size();
  ResourceRequest req = (*req_queue.begin()).second;

  if ((*workers)[worker_id].Reserve(req.GetResource())) {
    (*workers)[worker_id].IncreaseMemoryUsage(req.GetResource().GetMemory());
    req_queue.erase(req_queue.begin());
    return {{worker_id, req}};
  } else {
    return {};
  }
}

} // namespace simulation
} // namespace axe
