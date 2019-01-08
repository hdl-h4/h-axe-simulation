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

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <string>

#include "event.h"

namespace axe {
namespace simulation {

class EventQueue {
public:
  std::shared_ptr<Event> Top() const {return pq_.top();}
  void Pop() {pq_.pop();}
  void Push(std::shared_ptr<Event> event) {pq_.push(event);}
  bool Empty() const {return pq_.empty();}

private:
  std::priority_queue<std::shared_ptr<Event>> pq_;
  
};

EventQueue event_queue;

} // namespace simulation
} // namespace axe
