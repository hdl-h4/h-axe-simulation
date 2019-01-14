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

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "event/job_admission_event.h"
#include "event/new_task_req_event.h"
#include "event/placement_decision_event.h"
#include "event/resource_available_event.h"
#include "event/task_finish_event.h"
#include "event_handler.h"
#include "event_queue.h"
#include "job/job.h"
#include "job/shard_task.h"
#include "resource/resource.h"
#include "resource/resource_request.h"

namespace axe {
namespace simulation {

class JobManager : public EventHandler {
public:
  JobManager(const Job &job) : job_(job) {
    RegisterHandlers();
    BuildDependencies();
  }

  void RegisterHandlers() {
    RegisterHandler(
        JOB_ADMISSION,
        [&](const std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;
          for (const auto &req : GenerateResourceRequest(JOB_ADMISSION)) {
            event_vector.push_back(
                std::make_shared<NewTaskReqEvent>(NewTaskReqEvent(
                    NEW_TASK_REQ, global_clock, 0, SCHEDULER, req)));
          }
          return event_vector;
        });
    RegisterHandler(
        PLACEMENT_DECISION,
        [&](const std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;

          std::shared_ptr<PlacementDecisionEvent> decision =
              std::static_pointer_cast<PlacementDecisionEvent>(event);
          int worker_id = decision->GetWorkerId();
          int subgraph_id = decision->GetSubGraphId();
          job_.SetWorkerId(subgraph_id, worker_id);

          auto &sg = job_.GetSubGraphs().at(subgraph_id);
          for (auto &st : sg.GetShardTasks()) {
            if (dep_counter_[std::make_pair(st.GetTaskId(), st.GetShardId())] ==
                0) {
              bool success = workers.at(worker_id).PlaceNewTask(st);
              if (success) {
                event_vector.push_back(std::make_shared<TaskFinishEvent>(
                    TaskFinishEvent(TASK_FINISH,
                                    global_clock + st.GetDuration(), 0,
                                    job_.GetId(), st)));
              }
            }
          }
          return event_vector;
        });

    RegisterHandler(
        TASK_FINISH,
        [&](const std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;

          std::shared_ptr<TaskFinishEvent> task_finish_event =
              std::static_pointer_cast<TaskFinishEvent>(event);
          auto &finish_task = task_finish_event->GetShardTask();
          auto subgraph_id = shard_task_to_subgraph_[std::make_pair(
              finish_task.GetTaskId(), finish_task.GetShardId())];
          auto &sg = job_.GetSubGraphs().at(subgraph_id);

          // task finish update resource
          auto new_tasks = workers.at(sg.GetWorkerId()).TaskFinish(finish_task);
          for (auto &t : new_tasks) {
            event_vector.push_back(std::make_shared<TaskFinishEvent>(
                TaskFinishEvent(TASK_FINISH, global_clock + t.GetDuration(), 0,
                                job_.GetId(), t)));
          }

          subgraph_finished_task_[subgraph_id]++;
          // subgraph finish update memory
          if (sg.GetShardTasks().size() ==
              subgraph_finished_task_[subgraph_id]) {
            workers.at(sg.GetWorkerId()).SubGraphFinish(sg.GetMemory());
          }

          for (auto &child : finish_task.GetChildren()) {
            dep_finish_counter_[child]++;
            if (dep_finish_counter_[child] == dep_counter_[child]) {
              if (job_.GetSubGraphs()
                      .at(shard_task_to_subgraph_[child])
                      .GetWorkerId() == -1) {
                for (const auto &req :
                     GenerateResourceRequest(TASK_FINISH, child)) {
                  event_vector.push_back(
                      std::make_shared<NewTaskReqEvent>(NewTaskReqEvent(
                          NEW_TASK_REQ, global_clock, 0, SCHEDULER, req)));
                }
              } else {
                ShardTask &task = id_to_shard_task_[child];
                bool success = workers.at(sg.GetWorkerId()).PlaceNewTask(task);
                if (success) {
                  event_vector.push_back(std::make_shared<TaskFinishEvent>(
                      TaskFinishEvent(TASK_FINISH,
                                      global_clock + task.GetDuration(), 0,
                                      job_.GetId(), task)));
                }
              }
            }
          }

          // create resource available event
          event_vector.push_back(std::make_shared<ResourceAvailableEvent>(
              RESOURCE_AVAILABLE, global_clock, 0, SCHEDULER));
          return event_vector;
        });
  }

  std::vector<ResourceRequest> GenerateResourceRequest(
      EventType event_type,
      std::pair<int, int> shard_task = std::make_pair(-1, -1)) {
    std::vector<ResourceRequest> req_vector;
    if (event_type == EventType::JOB_ADMISSION) {
      for (int id = 0; id < job_.GetSubGraphs().size(); ++id) {
        auto &sg = job_.GetSubGraphs().at(id);
        for (auto &st : sg.GetShardTasks()) {
          if (dep_counter_[std::make_pair(st.GetTaskId(), st.GetShardId())] ==
              0) {
            subgraph_to_req_[id] = 1;
            req_vector.push_back(ResourceRequest(
                job_.GetId(), id, sg.GetDataLocality(), sg.GetResourcePack()));
            break;
          }
        }
      }
    } else {
      int id = shard_task_to_subgraph_[shard_task];
      if (subgraph_to_req_[id] == 1) {
        return req_vector;
      }
      subgraph_to_req_[id] = 1;
      auto &sg = job_.GetSubGraphs().at(id);
      req_vector.push_back(ResourceRequest(
          job_.GetId(), id, sg.GetDataLocality(), sg.GetResourcePack()));
    }
    return req_vector;
  }

  inline auto &GetJob() const { return job_; }

  void BuildDependencies() {
    for (int i = 0; i < job_.GetSubGraphs().size(); ++i) {
      auto &sg = job_.GetSubGraphs().at(i);
      for (auto &st : sg.GetShardTasks()) {
        shard_task_to_subgraph_[std::make_pair(st.GetTaskId(),
                                               st.GetShardId())] = i;
        for (auto &child : st.GetChildren()) {
          dep_counter_[child]++;
        }
      }
    }
  }

private:
  Job job_;
  std::map<int, int> subgraph_finished_task_;
  std::map<std::pair<int, int>, ShardTask> id_to_shard_task_;
  std::map<std::pair<int, int>, int> shard_task_to_subgraph_;
  std::unordered_map<int, bool> subgraph_to_req_;
  std::map<std::pair<int, int>, int> dep_counter_;
  std::map<std::pair<int, int>, int> dep_finish_counter_;
};

} // namespace simulation
} // namespace axe
