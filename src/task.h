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

#include <memory>
#include <string>
#include <vector>

namespace axe {
namespace simulation {

using nlohmann::json;

enum Resource { cpu = 1, memory, network, disk };
enum Dependency { Sync = 1, AsyncComm };

class Task {
public:
  Task() {}

  inline const auto GetId() const { return id_; }
  inline const auto GetParallelism() const { return parallelism_; }
  inline const auto &GetChildren() const { return children_; }
  inline const auto GetResource() const { return resource_; }
  inline const auto GetUsage() const { return usage_; }
  inline const auto GetDuration() const { return duration_; }
  inline const auto &GetLocality() const { return locality_; }

  friend void from_json(const json &j, Task &task) {
    j.at("id").get_to(task.id_);
    j.at("resource").get_to(task.resource_);
    j.at("usage").get_to(task.usage_);
    j.at("duration").get_to(task.duration_);
    j.at("parallelism").get_to(task.parallelism_);
    auto pos = j.find("partasks");
    if (pos != j.end()) {
      pos->get_to(task.children_);
    }
  }

private:
  int id_;
  Resource resource_;
  int usage_;
  int duration_;
  int parallelism_;
  std::vector<std::pair<int, Dependency>> children_;
  std::vector<std::shared_ptr<std::string>> locality_;
};

} // namespace simulation
} // namespace axe
