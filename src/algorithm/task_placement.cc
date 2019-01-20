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
  DLOG(INFO) << "task placement: FIFO and Round Robin";
  static int worker_id = -1;
  worker_id = (worker_id + 1) % (*workers).size();
  ResourceRequest req = (*req_queue.begin()).second;

  if ((*workers)[worker_id].Reserve(req.GetResource())) {
    req_queue.erase(req_queue.begin());
    return {{worker_id, req}};
  } else {
    return {};
  }
}

std::vector<std::pair<int, ResourceRequest>>
TETRIS(std::multimap<double, ResourceRequest> &req_queue,
       std::shared_ptr<std::vector<Worker>> workers) {
  DLOG(INFO) << "task placement: fifo and simplified tetris";
  int worker_id = -1;
  double max_product = 0;
  ResourceRequest req = (*req_queue.begin()).second;
  for (int i = 0; i < workers->size(); ++i) {
    if ((*workers)[i].TryToReserve(req.GetResource())) {
      double product = (*workers)[i].ComputeDotProduct(req.GetResource());
      if (product > max_product) {
        max_product = product;
        worker_id = i;
      }
    }
  }
  if (worker_id == -1)
    return {};
  else {
    DLOG(INFO) << "max product is: " << max_product;
    (*workers)[worker_id].Reserve(req.GetResource());
    req_queue.erase(req_queue.begin());
    return {{worker_id, req}};
  }
}

} // namespace simulation
} // namespace axe
