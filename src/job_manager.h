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

#include "glog/logging.h"

#include "event/job_admission_event.h"
#include "event/job_finish_event.h"
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
#include "user.h"
#include "worker/worker.h"

namespace axe {
namespace simulation {

class JobManager : public EventHandler {
public:
  JobManager(const Job &job, const std::shared_ptr<std::vector<Worker>> workers,
             const std::shared_ptr<std::vector<User>> users)
      : job_(job), workers_(workers), users_(users) {
    RegisterHandlers();
    BuildDependencies();
  }

  void RegisterHandlers() {
    RegisterHandler(
        EventType::JOB_ADMISSION, [&](std::shared_ptr<Event> event)
                                      -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;
          double time = event->GetTime();
          for (const auto &req :
               GenerateResourceRequest(EventType::JOB_ADMISSION)) {
            event_vector.push_back(
                std::make_shared<NewTaskReqEvent>(NewTaskReqEvent(
                    EventType::NEW_TASK_REQ, time, 0, kScheduler, req)));
          }
          return event_vector;
        });
    RegisterHandler(
        EventType::PLACEMENT_DECISION,
        [&](std::shared_ptr<Event> event)
            -> std::vector<std::shared_ptr<Event>> {
          std::vector<std::shared_ptr<Event>> event_vector;
          double time = event->GetTime();
          std::shared_ptr<PlacementDecisionEvent> decision =
              std::static_pointer_cast<PlacementDecisionEvent>(event);
          int worker_id = decision->GetWorkerID();
          int subgraph_id = decision->GetSubGraphID();
          job_.SetWorkerID(subgraph_id, worker_id);

          auto &sg = job_.GetSubGraphs().at(subgraph_id);
          // update user resource
          users_->at(job_.GetUserID())
              .PlacementDecision(time, sg.GetResourcePack());
          DLOG(INFO) << "the number of tasks is " << sg.GetShardTasks().size();
          for (auto &st : sg.GetShardTasks()) {
            DLOG(INFO) << "task memory of sg: " << subgraph_id << " is "
                       << st.GetMemory();
            ShardTaskID shard_task_id =
                ShardTaskID{st.GetTaskID(), st.GetShardID()};
            if (dep_counter_[shard_task_id] ==
                dep_finish_counter_[shard_task_id]) {
              std::vector<std::shared_ptr<Event>> events =
                  workers_->at(worker_id).PlaceNewTask(time, st);
              if (events.size() == 0)
                DLOG(INFO) << "event vector is empty";
              event_vector.insert(event_vector.end(), events.begin(),
                                  events.end());
            }
          }
          return event_vector;
        });

    RegisterHandler(EventType::TASK_FINISH, [&](std::shared_ptr<Event> event)
                                                -> std::vector<
                                                    std::shared_ptr<Event>> {
      std::vector<std::shared_ptr<Event>> event_vector;

      double time = event->GetTime();
      int event_id = event->GetEventID();
      std::shared_ptr<TaskFinishEvent> task_finish_event =
          std::static_pointer_cast<TaskFinishEvent>(event);
      auto &finish_task = task_finish_event->GetShardTask();
      auto subgraph_id = shard_task_to_subgraph_[ShardTaskID{
          finish_task.GetTaskID(), finish_task.GetShardID()}];
      auto &sg = job_.GetSubGraphs().at(subgraph_id);

      DLOG(INFO) << "TASK FINISH : subgraph id = " << subgraph_id
                 << ", task id = " << finish_task.GetTaskID()
                 << ", shard id = " << finish_task.GetShardID()
                 << ", worker id = " << sg.GetWorkerID();

      // task finish update resource

      std::vector<std::shared_ptr<Event>> events =
          workers_->at(sg.GetWorkerID())
              .TaskFinish(time, event_id, finish_task);
      event_vector.insert(event_vector.end(), events.begin(), events.end());
      users_->at(job_.GetUserID()).TaskFinish(time, finish_task);

      subgraph_finished_task_[subgraph_id]++;
      // subgraph finish update memory
      if (sg.GetShardTasks().size() == subgraph_finished_task_[subgraph_id]) {
        workers_->at(sg.GetWorkerID()).SubGraphFinish(time, sg.GetMemory());
        users_->at(job_.GetUserID()).SubGraphFinish(time, sg.GetMemory());
        DLOG(INFO) << "finish subgraph: " << subgraph_id
                   << " of job id: " << sg.GetJobID();
        finish_subgraph_num_++;
        if (finish_subgraph_num_ == job_.GetSubGraphs().size()) {
          event_vector.push_back(std::make_shared<JobFinishEvent>(
              EventType::JOB_FINISH, time, 0, kScheduler, job_.GetJobID(),
              job_.GetSubmissionTime()));
        }
      }

      for (auto &child : finish_task.GetChildren()) {
        dep_finish_counter_[child]++;
        int child_subgraph_id = shard_task_to_subgraph_[child];
        subgraph_dep_finish_counter_[child_subgraph_id]++;
        if (dep_finish_counter_[child] == dep_counter_[child]) {
          if (job_.GetSubGraphs().at(child_subgraph_id).GetWorkerID() == -1) {
            if (subgraph_dep_finish_counter_[child_subgraph_id] ==
                subgraph_dep_counter_[child_subgraph_id]) {
              for (const auto &req :
                   GenerateResourceRequest(EventType::TASK_FINISH, child)) {
                event_vector.push_back(
                    std::make_shared<NewTaskReqEvent>(NewTaskReqEvent(
                        EventType::NEW_TASK_REQ, time, 0, kScheduler, req)));
              }
            }
          } else {
            ShardTask task = id_to_shard_task_[child];
            std::vector<std::shared_ptr<Event>> update_event_vector =
                workers_
                    ->at(
                        job_.GetSubGraphs().at(child_subgraph_id).GetWorkerID())
                    .PlaceNewTask(time, task);
            event_vector.insert(event_vector.end(), update_event_vector.begin(),
                                update_event_vector.end());
          }
        }
      }

      // create resource available event
      event_vector.push_back(std::make_shared<ResourceAvailableEvent>(
          EventType::RESOURCE_AVAILABLE, time, 0, kScheduler));
      return event_vector;
    });
  }

  std::vector<ResourceRequest>
  GenerateResourceRequest(EventType event_type,
                          ShardTaskID shard_task = ShardTaskID{-1, -1}) {
    std::vector<ResourceRequest> req_vector;
    if (event_type == EventType::JOB_ADMISSION) {
      for (int id = 0; id < job_.GetSubGraphs().size(); ++id) {
        auto &sg = job_.GetSubGraphs().at(id);
        for (auto &st : sg.GetShardTasks()) {
          if (dep_counter_[ShardTaskID{st.GetTaskID(), st.GetShardID()}] == 0) {
            subgraph_to_req_[id] = 1;
            req_vector.push_back(ResourceRequest(job_.GetJobID(), id,
                                                 sg.GetDataLocality(),
                                                 sg.GetResourcePack()));
            break;
          }
        }
      }
    } else {
      int id = shard_task_to_subgraph_[shard_task];
      DLOG(INFO) << "subgraph id : " << id;
      if (subgraph_to_req_[id] == 1) {
        return req_vector;
      }
      subgraph_to_req_[id] = 1;
      auto &sg = job_.GetSubGraphs().at(id);
      req_vector.push_back(ResourceRequest(
          job_.GetJobID(), id, sg.GetDataLocality(), sg.GetResourcePack()));
    }
    return req_vector;
  }

  inline auto &GetJob() const { return job_; }

  void BuildDependencies() {
    for (int i = 0; i < job_.GetSubGraphs().size(); ++i) {
      const auto &sg = job_.GetSubGraphs().at(i);
      for (const auto &st : sg.GetShardTasks()) {
        shard_task_to_subgraph_[ShardTaskID{st.GetTaskID(), st.GetShardID()}] =
            i;
        subgraph_finished_task_[i] = 0;
        id_to_shard_task_[ShardTaskID{st.GetTaskID(), st.GetShardID()}] = st;
        for (const auto &child : st.GetChildren()) {
          dep_counter_[child]++;
        }
      }
    }
    for (int i = 0; i < job_.GetSubGraphs().size(); i++) {
      const auto &sg = job_.GetSubGraphs().at(i);
      for (const auto &st : sg.GetShardTasks()) {
        for (const auto &child : st.GetChildren()) {
          if (shard_task_to_subgraph_[child] != i) {
            subgraph_dep_counter_[shard_task_to_subgraph_[child]]++;
          }
        }
      }
    }
  }

private:
  Job job_;
  int finish_subgraph_num_ = 0;
  std::map<int, int> subgraph_finished_task_;
  std::map<ShardTaskID, ShardTask> id_to_shard_task_;
  std::map<ShardTaskID, int> shard_task_to_subgraph_;
  std::unordered_map<int, bool> subgraph_to_req_;
  std::map<ShardTaskID, int> dep_counter_;        // for task
  std::map<ShardTaskID, int> dep_finish_counter_; // for task
  std::map<int, int> subgraph_dep_counter_;
  std::map<int, int> subgraph_dep_finish_counter_;
  std::shared_ptr<std::vector<Worker>> workers_;
  std::shared_ptr<std::vector<User>> users_;
}; // namespace simulation

} // namespace simulation
} // namespace axe
