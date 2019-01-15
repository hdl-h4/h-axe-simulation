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

#include "algorithm_book.h"
#include "resource/resource_request.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace axe {
namespace simulation {

std::string AlgorithmBook::task_placement = "";
std::map<std::string,
         std::function<std::vector<std::pair<int, ResourceRequest>>(
             std::multimap<double, ResourceRequest> &,
             std::shared_ptr<std::vector<Worker>>)>>
    AlgorithmBook::task_placement_book = {};

} // namespace simulation
} // namespace axe
