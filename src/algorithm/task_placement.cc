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
     std::vector<std::shared_ptr<WorkerAbstract>> &workers) {
  DLOG(INFO) << "task placement: FIFO and Round Robin";
  std::vector<std::pair<int, ResourceRequest>> placement_decision_;
  static int worker_id = -1;
  ResourceRequest req = (*req_queue.begin()).second;
  worker_id = (worker_id + 1) % workers.size();

  /*
  if(req.IsCPURequest()) {
    std::cout << "req cpu: " << req.GetResource().GetCPU() << std::endl;
    std::cout << "req mem: " << req.GetResource().GetMemory() << std::endl <<
  std::endl;
    std::cout << "-----------------------------------------------------" <<
  std::endl << std::endl;
    for(int i=0; i< workers.size(); ++i) {
      std::cout << "worker " << i << std::endl;
      workers[i]->ReportCPUStatus();
    }
    std::cout << "-----------------------------------------------------" <<
  std::endl << std::endl;
  }
  */

  DLOG(INFO) << "reserve on worker " << worker_id;
  while (workers.at(worker_id)->Reserve(req.GetResource())) {

    // std::cout << "[!]reserve on worker " << worker_id << std::endl;

    DLOG(INFO) << "reserve succefully";
    req_queue.erase(req_queue.begin());
    placement_decision_.push_back({worker_id, req});
    if (req_queue.size() == 0) {
      break;
    }
    worker_id = (worker_id + 1) % workers.size();
    req = (*req_queue.begin()).second;
    DLOG(INFO) << "reserve on worker " << worker_id;

    /*
    if(req.IsCPURequest()) {
      std::cout << "req cpu: " << req.GetResource().GetCPU() << std::endl;
      std::cout << "req mem: " << req.GetResource().GetMemory() << std::endl <<
    std::endl;;
      std::cout << "-----------------------------------------------------" <<
    std::endl << std::endl;
      for(int i=0; i< workers.size(); ++i) {
        std::cout << "worker " << i << std::endl;
        workers[i]->ReportCPUStatus();
      }
      std::cout << "-----------------------------------------------------" <<
    std::endl << std::endl;
    }
    */
  }
  return placement_decision_;
}

std::vector<std::pair<int, ResourceRequest>>
TETRIS(std::multimap<double, ResourceRequest> &req_queue,
       std::vector<std::shared_ptr<WorkerAbstract>> &workers) {
  DLOG(INFO) << "task placement: fifo and simplified tetris";
  int worker_id = -1;
  double max_product = 0;
  ResourceRequest req = (*req_queue.begin()).second;
  for (int i = 0; i < workers.size(); ++i) {
    if (workers[i]->TryToReserve(req.GetResource())) {
      double product =
          workers[i]->GetAvailableResource().DotProduct(req.GetResource());
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
    workers[worker_id]->Reserve(req.GetResource());
    req_queue.erase(req_queue.begin());
    return {{worker_id, req}};
  }
}

} // namespace simulation
} // namespace axe
