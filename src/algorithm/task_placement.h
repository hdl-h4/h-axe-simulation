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

#include "resource/resource_request.h"
#include "worker/worker.h"
#include <map>
#include <queue>
#include <vector>

namespace axe {
namespace simulation {

std::vector<std::pair<int, ResourceRequest>>
FIFO(std::multimap<double, ResourceRequest> &req_queue,
     std::vector<std::shared_ptr<WorkerAbstract>> &);

std::vector<std::pair<int, ResourceRequest>>
TETRIS(std::multimap<double, ResourceRequest> &req_queue,
       std::vector<std::shared_ptr<WorkerAbstract>> &);

} // namespace simulation
} // namespace axe
