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

#include "nlohmann/json.hpp"

namespace axe {
namespace simulation {

using nlohmann::json;

class Worker {
public:
  Worker() = default;

  friend void from_json(const json &j, Worker &worker) {
    j.at("id").get_to(worker.id_);
    j.at("cpu").get_to(worker.cpu_);
    j.at("memory").get_to(worker.memory_);
    j.at("disk").get_to(worker.disk_);
    j.at("network").get_to(worker.net_);
  }

private:
  int id_;
  int cpu_;
  int memory_;
  int disk_;
  int net_;
};

} // namespace simulation
} // namespace axe
