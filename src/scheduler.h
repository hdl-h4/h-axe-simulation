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
#include <string>
#include <queue>
#include <memory>
#include <map>
#include "event.h"
#include "worker.h"
#include "event_handler.h"
#include "event_queue.h"

namespace axe{
namespace simulation{

class Scheduler : public EventHandler{

public:
  Scheduler() {
    RegisterHandler();
  }

  Scheduler(const std::string& workers_desc) {
    SetWorkers(workers_desc);
    RegisterHandler();
  }

  void RegisterHandler() {
    handler_map_.insert({NEW_JOB, [=](const std::shared_ptr<Event> event){
      //Job admission control
    }});
    handler_map_.insert({JOB_FINISH, [=](const std::shared_ptr<Event> event){
      
    }});
    handler_map_.insert({NEW_TASK_REQ, [=](const std::shared_ptr<Event> event){
      //Job admission control
    }});
  }

  void Handle(const std::shared_ptr<Event> event) {
    //TODO(SXD): handle function for Scheduler
    handler_map_[event->GetEventType()](event);
  }

  void SetWorkers(const std::string& workers_desc) {
    //TODO(LBY): generate the workers picture
  }


private:
  std::vector<Worker> workers_;
  std::map<int, std::function<void(const std::shared_ptr<Event> event)>> handler_map_;
};

}  //namespace simulation
}  //namespace axe
