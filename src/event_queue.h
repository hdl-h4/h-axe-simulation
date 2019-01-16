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

#include "event/event.h"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace axe {
namespace simulation {

struct cmp {
  bool operator()(const std::shared_ptr<Event> &lhs,
                  const std::shared_ptr<Event> &rhs) {
    return (*rhs) < (*lhs);
  }
};

class EventQueue {
public:
  std::shared_ptr<Event> Top() const { return pq_.top(); }
  void Pop() { pq_.pop(); }
  void Push(std::shared_ptr<Event> event) { pq_.push(event); }
  void Push(std::vector<std::shared_ptr<Event>> event_vector) {
    for (const auto &e : event_vector) {
      pq_.push(e);
    }
  }
  bool Empty() const { return pq_.empty(); }

private:
  std::priority_queue<std::shared_ptr<Event>,
                      std::vector<std::shared_ptr<Event>>, cmp>
      pq_;
};

} // namespace simulation
} // namespace axe
