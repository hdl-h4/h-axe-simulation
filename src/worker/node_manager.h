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

#include "worker_abstract.h"

namespace axe {
namespace simulation {

using nlohmann::json;

class NodeManager : public WorkerAbstract {
public:
  NodeManager() {}
  NodeManager(double cpu, double memory, double disk, double network) {
    resource_capacity_ =
        std::make_shared<ResourcePack>(cpu, memory, disk, network);
    resource_usage_ = std::make_shared<ResourcePack>();
    resource_reservation_ = std::make_shared<ResourcePack>();
  }

  void Init(int worker_id,
            std::shared_ptr<std::set<int>> invalid_event_id_set) {
    worker_id_ = worker_id;
    invalid_event_id_set_ = invalid_event_id_set;
    CHECK(false) << "stop here";
    worker_cpu_ = WorkerCPU(worker_id_, resource_capacity_, resource_usage_,
                            resource_reservation_, false);
    worker_disk_ =
        WorkerDisk(worker_id_, resource_capacity_, resource_usage_,
                   resource_reservation_, invalid_event_id_set_, false);
    worker_network_ =
        WorkerNetwork(worker_id_, resource_capacity_, resource_usage_,
                      resource_reservation_, invalid_event_id_set_, false);
  }

  friend void from_json(const json &j, NodeManager &manager) {
    manager.resource_capacity_ = std::make_shared<ResourcePack>();
    manager.resource_usage_ = std::make_shared<ResourcePack>();
    manager.resource_reservation_ = std::make_shared<ResourcePack>();
    j.get_to(*(manager.resource_capacity_));
    manager.records_.insert(manager.GenerateUtilizationRecord(0));
  }

  // subgraph finish
  void SubGraphFinish(double time, ResourcePack resource) {
    DLOG(INFO) << "sg finish, release cpu and memory container";
    resource_reservation_->SubtractFromMe(resource);
    records_.insert(GenerateUtilizationRecord(time));
  }

  bool Reserve(ResourcePack resource) {
    if (resource_reservation_->Add(resource).FitIn(*resource_capacity_)) {
      DLOG(INFO) << "resource memory reservation now is "
                 << resource_reservation_->GetMemory() << " will increase by "
                 << resource.GetMemory();
      resource_reservation_->AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

  bool TryToReserve(ResourcePack resource) {
    return resource_reservation_->Add(resource).FitIn(*resource_capacity_);
  }

private:
  // std::shared_ptr<ResourcePack> resource_capacity_;
  /* Capacity: capacity describes the physical resources on the worker.
   *   CPU: the number of cpu cores on this worker (e.g., 8 cores).
   *   Memory: the size of memory on this worker (e.g., 16GB).
   *   Disk: the bandwidth of disk on this worker (e.g., 100MB/s).
   *   Network: the bandwidth of network on this worker (e.g., 1GB/s).
   */

  // std::shared_ptr<ResourcePack> resource_usage_;
  /* Usage: usage describes the resources currently in use on the worker. */

  // std::shared_ptr<ResourcePack> resource_reservation_;
  /* Reservation: reservation describes the resources reserved (for container)
   * on the worker. */
};

} //  namespace simulation
} //  namespace axe
