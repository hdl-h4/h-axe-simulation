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
#include <memory>
#include "job.h"
#include "event.h"
#include "event_handler.h"
#include "event_queue.h"

namespace axe{
namespace simulation{

class JobManager : public EventHandler {
public:
  JobManager() {
    RegisterHandler();
  }

  JobManager(const std::string& job_desc) {
    SetJob(job_desc);
    RegisterHandler();
  }

  void RegisterHandler() {
    handler_map_.insert({TASK_REQ_FINISH, [=](const std::shared_ptr<Event> event){
      
    }});
  }

  void Handle(const std::shared_ptr<Event> event) {
    //TODO(SXD): handle function for JM
    handler_map_[event->GetEventType()](event);
  }
  
  void SetJob(const std::string& job_desc) {
    //TODO(LBY): generate the (physical) job picture
    
  }

  std::shared_ptr<Job> GetJob() const {return job_;}

private:
  std::shared_ptr<Job> job_;
  std::map<int, std::function<void(const std::shared_ptr<Event> event)>> handler_map_;
};

}  //namespace simulation
}  //namespace axe
