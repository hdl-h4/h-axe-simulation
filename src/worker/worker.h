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

class Worker : public WorkerAbstract {
public:
  Worker() {}

  Worker(double cpu, double memory, double disk, double network) {
    // CHECK(false) << "don't support this worker constructor now";
    resource_capacity_ =
        std::make_shared<ResourcePack>(cpu, memory, disk, network);
    resource_usage_ = std::make_shared<ResourcePack>();
    resource_reservation_ = std::make_shared<ResourcePack>();
    resource_maximum_reservation_ = std::make_shared<ResourcePack>(
        2000, resource_capacity_->GetMemory(), 2000, 2000);
  }

  void Init(int worker_id,
            std::shared_ptr<std::set<int>> invalid_event_id_set) {

    worker_id_ = worker_id;
    invalid_event_id_set_ = invalid_event_id_set;
    worker_cpu_ = WorkerCPU(worker_id_, resource_capacity_, resource_usage_,
                            resource_reservation_, true);
    worker_disk_ =
        WorkerDisk(worker_id_, resource_capacity_, resource_usage_,
                   resource_reservation_, invalid_event_id_set_, true);
    worker_network_ =
        WorkerNetwork(worker_id_, resource_capacity_, resource_usage_,
                      resource_reservation_, invalid_event_id_set_, true);
  }

  void ReportCPUStatus() {
    std::cout << "cpu reservation limit is "
              << resource_maximum_reservation_->GetCPU() << std::endl;
    std::cout << "cpu reservation is " << resource_reservation_->GetCPU()
              << std::endl;
    std::cout << "cpu capacity is " << resource_capacity_->GetCPU()
              << std::endl;
    std::cout << "cpu usage is " << resource_usage_->GetCPU() << std::endl;

    std::cout << "memory reservation limit is "
              << resource_maximum_reservation_->GetMemory() << std::endl;
    std::cout << "memory reservation is " << resource_reservation_->GetMemory()
              << std::endl;
    std::cout << "memory capacity is " << resource_capacity_->GetMemory()
              << std::endl;
    std::cout << "memory usage is " << resource_usage_->GetMemory()
              << std::endl;
  }

  // subgraph finish
  void SubGraphFinish(double time, ResourcePack resource) {
    DLOG(INFO) << "sg finish, release memory reservation: "
               << resource.GetMemory();
    resource_reservation_->SetMemory(resource_reservation_->GetMemory() -
                                     resource.GetMemory());
    records_.insert(GenerateUtilizationRecord(time));
  }

  void Print() {
    DLOG(INFO) << "cpu: " << resource_capacity_->GetCPU();
    DLOG(INFO) << "mem: " << resource_capacity_->GetMemory();
    DLOG(INFO) << "disk: " << resource_capacity_->GetDisk();
    DLOG(INFO) << "net: " << resource_capacity_->GetNetwork();
  }

  void PrintReservation() {
    DLOG(INFO) << "cpu: " << resource_reservation_->GetCPU();
    DLOG(INFO) << "mem: " << resource_reservation_->GetMemory();
    DLOG(INFO) << "disk: " << resource_reservation_->GetDisk();
    DLOG(INFO) << "net: " << resource_reservation_->GetNetwork();
  }

  bool Reserve(ResourcePack resource) {
    if (resource_reservation_->Add(resource).FitIn(
            *resource_maximum_reservation_)) {
      DLOG(INFO) << "resource memory reservation now is "
                 << resource_reservation_->GetMemory() << " will increase by "
                 << resource.GetMemory();
      DLOG(INFO) << "resource cpu reservation now is "
                 << resource_reservation_->GetCPU() << " will increase by "
                 << resource.GetCPU();
      resource_reservation_->AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

  bool WeakReserve(ResourcePack resource) {
    if (resource_reservation_->Add(resource).WeakFitIn(
            *resource_maximum_reservation_, oversell_factor_)) {
      resource_reservation_->AddToMe(resource);
      return true;
    } else {
      return false;
    }
  }

  bool TryToReserve(ResourcePack resource) {
    return resource_reservation_->Add(resource).FitIn(
        *resource_maximum_reservation_);
  }

  friend void from_json(const json &j, std::shared_ptr<Worker> &worker) {
    worker = std::make_shared<Worker>();
    worker->resource_capacity_ = std::make_shared<ResourcePack>();
    worker->resource_usage_ = std::make_shared<ResourcePack>();
    worker->resource_reservation_ = std::make_shared<ResourcePack>();
    j.get_to(*(worker->resource_capacity_));
    worker->resource_maximum_reservation_ = std::make_shared<ResourcePack>();
    *(worker->resource_maximum_reservation_) = {
        50000000, worker->resource_capacity_->GetMemory(), 1000000, 1000000};
    worker->records_.insert(worker->GenerateUtilizationRecord(0));
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
  /* Usage: usage describes the resource utilization at this moment which is
   * considered in local queue management.
   *    CPU: the number of cpu cores in use on this worker (e.g., 4 cores in
   * use).
   *    Memory: the size of memory in use on this worker (e.g., 10GB in use).
   *    Disk: the share of disk bandwidth on the worker (e.g., 75MB/s if you
   * have 3 disk tasks sharing a 100MB/s bandwidth and the maximum disk task
   * number is 4).
   *    Network: the share of network bandwidth on this worker (e.g.,
   * 0.5GB/s if you have 5 network tasks sharing a 1GB/s bandwidth and the
   * maximum network task number is 10).
   */
  // std::shared_ptr<ResourcePack> resource_reservation_;
  /* Reservation: reservation is a coarse description of the expected workload
   * on this worker which is considered in global scheduling.
   *    CPU: the total CPU workload reserved on this worker (the total workload
   * means the accumulation of cpu work time of every cpu task).
   *    Memory: the size of memory reserved on this worker (e.g., 10 GB in
   * total and 8GB reserved).
   *    Disk: the total disk workload reserved on this worker (the total
   * workload means the accumulation of data size of every disk task).
   *    Network: the total network workload reserved on this worker (same as
   * Disk).
   */
  std::shared_ptr<ResourcePack> resource_maximum_reservation_;
  // the maximum reservation which is a upper bound during Reserve()

  double oversell_factor_ = 1.5;
};

} //  namespace simulation
} //  namespace axe
