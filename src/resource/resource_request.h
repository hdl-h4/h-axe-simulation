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

#include <vector>

#include "resource.h"

namespace axe {
namespace simulation {

class ResourceRequest {
public:
  ResourceRequest() {}
  ResourceRequest(int job_id, int subgraph_id, std::vector<int> data_locality,
                  ResourcePack resource)
      : job_id_(job_id), subgraph_id_(subgraph_id),
        data_locality_(data_locality), resource_(resource) {}

  inline int GetJobID() const { return job_id_; }
  inline int GetSubGraphID() const { return subgraph_id_; }
  inline const auto &GetDataLocality() const { return data_locality_; }
  inline const auto &GetResource() const { return resource_; }
  bool IsCPURequest() { return resource_.GetCPU() > 0; }

private:
  int job_id_;
  int subgraph_id_;
  std::vector<int> data_locality_;
  ResourcePack resource_;
};

} // namespace simulation
} // namespace axe
