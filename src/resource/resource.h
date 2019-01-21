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
#include <vector>

namespace axe {
namespace simulation {

using nlohmann::json;

enum class ResourceType: int { kCPU = 0, kMemory, kDisk, kNetwork };

const int kNumResourceTypes = 4;

class ResourcePack {
public:
  ResourcePack() {
    resource_.resize(kNumResourceTypes);
    for (int i = 0; i < resource_.size(); ++i) {
      resource_[i] = 0.0;
    }
  }

  ResourcePack(double cpu, double memory, double disk, double network) {
    resource_.resize(kNumResourceTypes);
    resource_[static_cast<int>(ResourceType::kCPU)] = cpu;
    resource_[static_cast<int>(ResourceType::kMemory)] = memory;
    resource_[static_cast<int>(ResourceType::kDisk)] = disk;
    resource_[static_cast<int>(ResourceType::kNetwork)] = network;
  }

  ResourcePack(std::vector<double> resource) {
    CHECK_EQ(resource.size(), kNumResourceTypes)
        << "invalid resource vector!!!";
    resource_ = resource;
  }

  void Print() {
    DLOG(INFO) << " cpu = " << resource_[static_cast<int>(ResourceType::kCPU)];
    DLOG(INFO) << " memory = " << resource_[static_cast<int>(ResourceType::kMemory)];
    DLOG(INFO) << " disk = " << resource_[static_cast<int>(ResourceType::kDisk)];
    DLOG(INFO) << " network = " << resource_[static_cast<int>(ResourceType::kNetwork)];
  }

  double GetCPU() const { return resource_[static_cast<int>(ResourceType::kCPU)]; }
  double GetMemory() const { return resource_[static_cast<int>(ResourceType::kMemory)]; }
  double GetDisk() const { return resource_[static_cast<int>(ResourceType::kDisk)]; }
  double GetNetwork() const { return resource_[static_cast<int>(ResourceType::kNetwork)]; }
  std::vector<double> GetResourceVector() const { return resource_; }
  double GetResourceByIndex(int idx) const { return resource_[idx]; }

  void SetCPU(double cpu) { resource_[static_cast<int>(ResourceType::kCPU)] = cpu; }
  void SetMemory(double memory) { resource_[static_cast<int>(ResourceType::kMemory)] = memory; }
  void SetDisk(double disk) { resource_[static_cast<int>(ResourceType::kDisk)] = disk; }
  void SetNetwork(double network) { resource_[static_cast<int>(ResourceType::kNetwork)] = network; }

  ResourcePack Add(const ResourcePack &rhs) const {
    ResourcePack result;
    result.SetCPU(resource_[static_cast<int>(ResourceType::kCPU)] + rhs.GetCPU());
    result.SetMemory(resource_[static_cast<int>(ResourceType::kMemory)] + rhs.GetMemory());
    result.SetDisk(resource_[static_cast<int>(ResourceType::kDisk)] + rhs.GetDisk());
    result.SetNetwork(resource_[static_cast<int>(ResourceType::kNetwork)] + rhs.GetNetwork());
    return result;
  }

  ResourcePack Subtract(const ResourcePack &rhs) const {
    ResourcePack result = SubtractWithoutMemory(rhs);
    result.SetMemory(resource_[static_cast<int>(ResourceType::kMemory)] - rhs.GetMemory());
    return result;
  }

  ResourcePack SubtractWithoutMemory(const ResourcePack &rhs) const {
    ResourcePack result;
    result.SetCPU(resource_[static_cast<int>(ResourceType::kCPU)] - rhs.GetCPU());
    result.SetDisk(resource_[static_cast<int>(ResourceType::kDisk)] - rhs.GetDisk());
    result.SetNetwork(resource_[static_cast<int>(ResourceType::kNetwork)] - rhs.GetNetwork());
    return result;
  }

  void AddToMe(const ResourcePack &rhs) {
    resource_[static_cast<int>(ResourceType::kCPU)] += rhs.GetCPU();
    resource_[static_cast<int>(ResourceType::kMemory)] += rhs.GetMemory();
    resource_[static_cast<int>(ResourceType::kDisk)] += rhs.GetDisk();
    resource_[static_cast<int>(ResourceType::kNetwork)] += rhs.GetNetwork();
  }

  void SubtractFromMe(const ResourcePack &rhs) {
    SubtractFromMeWithoutMemory(rhs);
    resource_[static_cast<int>(ResourceType::kMemory)] -= rhs.GetMemory();
  }

  void SubtractFromMeWithoutMemory(const ResourcePack &rhs) {
    resource_[static_cast<int>(ResourceType::kCPU)] -= rhs.GetCPU();
    resource_[static_cast<int>(ResourceType::kDisk)] -= rhs.GetDisk();
    resource_[static_cast<int>(ResourceType::kNetwork)] -= rhs.GetNetwork();
  }

  bool FitIn(const ResourcePack &resource) {
    bool ret = true;
    for (int i = 0; i < kNumResourceTypes; ++i) {
      ret = ret && (resource.GetResourceByIndex(i) < resource_[i]);
    }
  }

  bool WeakFitIn(const ResourcePack &resource, double alpha) {
    bool ret = true;
    for (int i = 0; i < kNumResourceTypes; ++i) {
      if (i == static_cast<int>(ResourceType::kMemory))
        ret = ret && (resource.GetResourceByIndex(i) < resource_[i]);
      else
        ret = ret && (resource.GetResourceByIndex(i) < resource_[i] * alpha);
    }
  }

  friend void from_json(const json &j, ResourcePack &resource_pack) {
    resource_pack.resource_.resize(kNumResourceTypes);
    j.at("cpu").get_to(resource_pack.resource_[static_cast<int>(ResourceType::kCPU)]);
    j.at("memory").get_to(resource_pack.resource_[static_cast<int>(ResourceType::kMemory)]);
    j.at("disk").get_to(resource_pack.resource_[static_cast<int>(ResourceType::kDisk)]);
    j.at("network").get_to(resource_pack.resource_[static_cast<int>(ResourceType::kNetwork)]);
  }

private:
  std::vector<double> resource_;
};
} // namespace simulation
} // namespace axe
