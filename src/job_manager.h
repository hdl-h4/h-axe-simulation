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

#include "event/job_admission_event.h"
#include "event/job_finish_event.h"
#include "event/new_task_req_event.h"
#include "event/placement_decision_event.h"
#include "event/resource_available_event.h"
#include "event/task_finish_event.h"
#include "event_handler.h"
#include "event_queue.h"
#include "glog/logging.h"
#include "job/job.h"
#include "job/shard_task.h"
#include "resource/resource.h"
#include "resource/resource_request.h"
#include "worker.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace axe {
namespace simulation {

class JobManager : public EventHandler {
public:
  JobManager(const Job &job, const std::shared_ptr<std::vector<Worker>> workers)
      : job_(job), workers_(workers) {
    RegisterHandlers();
    BuildDependencies();
  }

  void RegisterHandlers() {
    RegisterHandler(
        JOB_ADMISSION,
        [&](std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;
          double time = event->GetTime();
          for (const auto &req : GenerateResourceRequest(JOB_ADMISSION)) {
            event_vector.push_back(std::make_shared<NewTaskReqEvent>(
                NewTaskReqEvent(NEW_TASK_REQ, time, 0, SCHEDULER, req)));
          }
          return event_vector;
        });
    RegisterHandler(
        PLACEMENT_DECISION,
        [&](std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;
          double time = event->GetTime();
          std::shared_ptr<PlacementDecisionEvent> decision =
              std::static_pointer_cast<PlacementDecisionEvent>(event);
          int worker_id = decision->GetWorkerId();
          int subgraph_id = decision->GetSubGraphId();
          job_.SetWorkerId(subgraph_id, worker_id);

          auto &sg = job_.GetSubGraphs().at(subgraph_id);
          for (auto &st : sg.GetShardTasks()) {
            if (dep_counter_[ShardTaskId{st.GetTaskId(), st.GetShardId()}] ==
                0) {
              bool success = workers_->at(worker_id).PlaceNewTask(time, st);
              if (success) {
                event_vector.push_back(std::make_shared<TaskFinishEvent>(
                    TaskFinishEvent(TASK_FINISH, time + st.GetDuration(), 0,
                                    job_.GetId(), st)));
              }
            }
          }
          return event_vector;
        });

    RegisterHandler(
        TASK_FINISH,
        [&](std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;

          double time = event->GetTime();
          std::shared_ptr<TaskFinishEvent> task_finish_event =
              std::static_pointer_cast<TaskFinishEvent>(event);
          auto &finish_task = task_finish_event->GetShardTask();
          auto subgraph_id = shard_task_to_subgraph_[ShardTaskId{
              finish_task.GetTaskId(), finish_task.GetShardId()}];
          auto &sg = job_.GetSubGraphs().at(subgraph_id);

          // task finish update resource
          auto new_tasks =
              workers_->at(sg.GetWorkerId()).TaskFinish(time, finish_task);
          for (auto &t : new_tasks) {
            event_vector.push_back(
                std::make_shared<TaskFinishEvent>(TaskFinishEvent(
                    TASK_FINISH, time + t.GetDuration(), 0, job_.GetId(), t)));
          }

          subgraph_finished_task_[subgraph_id]++;
          // subgraph finish update memory
          if (sg.GetShardTasks().size() ==
              subgraph_finished_task_[subgraph_id]) {
            workers_->at(sg.GetWorkerId()).SubGraphFinish(sg.GetMemory());
            finish_subgraph_num_++;
            /*if(finish_subgraph_num_ == job_.GetSubGraphs().size()) {
              event_vector.push_back(std::make_shared<JobFinishEvent>(JobFinishEvent(JOB_FINISH,
            global_clock, 0, job_.GetId())));
              event_vector.push_back(std::make_shared<ResourceAvailableEvent>(RESOURCE_AVAILABLE,
            global_clock, 0, SCHEDULER)); return event_vector;
            }*/
          }

          for (auto &child : finish_task.GetChildren()) {
            dep_finish_counter_[child]++;
            if (dep_finish_counter_[child] == dep_counter_[child]) {
              if (job_.GetSubGraphs()
                      .at(shard_task_to_subgraph_[child])
                      .GetWorkerId() == -1) {
                for (const auto &req :
                     GenerateResourceRequest(TASK_FINISH, child)) {
                  event_vector.push_back(std::make_shared<NewTaskReqEvent>(
                      NewTaskReqEvent(NEW_TASK_REQ, time, 0, SCHEDULER, req)));
                }
              } else {
                ShardTask task = id_to_shard_task_[child];
                bool success = workers_
                                   ->at(job_.GetSubGraphs()
                                            .at(shard_task_to_subgraph_[child])
                                            .GetWorkerId())
                                   .PlaceNewTask(time, task);
                if (success) {
                  DLOG(INFO) << "subgraph id " << shard_task_to_subgraph_[child]
                             << '\n';
                  event_vector.push_back(std::make_shared<TaskFinishEvent>(
                      TaskFinishEvent(TASK_FINISH, time + task.GetDuration(), 0,
                                      job_.GetId(), task)));
                }
              }
            }
          }

          // create resource available event
          event_vector.push_back(std::make_shared<ResourceAvailableEvent>(
              RESOURCE_AVAILABLE, time, 0, SCHEDULER));
          return event_vector;
        });
  }

  std::vector<ResourceRequest>
  GenerateResourceRequest(EventType event_type,
                          ShardTaskId shard_task = ShardTaskId{-1, -1}) {
    std::vector<ResourceRequest> req_vector;
    if (event_type == EventType::JOB_ADMISSION) {
      for (int id = 0; id < job_.GetSubGraphs().size(); ++id) {
        auto &sg = job_.GetSubGraphs().at(id);
        for (auto &st : sg.GetShardTasks()) {
          if (dep_counter_[ShardTaskId{st.GetTaskId(), st.GetShardId()}] == 0) {
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
      const auto &sg = job_.GetSubGraphs().at(i);
      for (const auto &st : sg.GetShardTasks()) {
        shard_task_to_subgraph_[ShardTaskId{st.GetTaskId(), st.GetShardId()}] =
            i;
        id_to_shard_task_[ShardTaskId{st.GetTaskId(), st.GetShardId()}] = st;
        for (const auto &child : st.GetChildren()) {
          dep_counter_[child]++;
        }
      }
    }
  }

private:
  Job job_;
  int finish_subgraph_num_ = 0;
  std::map<int, int> subgraph_finished_task_;
  std::map<ShardTaskId, ShardTask> id_to_shard_task_;
  std::map<ShardTaskId, int> shard_task_to_subgraph_;
  std::unordered_map<int, bool> subgraph_to_req_;
  std::map<ShardTaskId, int> dep_counter_;
  std::map<ShardTaskId, int> dep_finish_counter_;
  std::shared_ptr<std::vector<Worker>> workers_;
};

} // namespace simulation
} // namespace axe
