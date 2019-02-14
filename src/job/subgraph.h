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

#include <queue>

#include "glog/logging.h"

#include "resource/resource.h"
#include "shard_task.h"

namespace axe {
namespace simulation {

const double inf = 1e12;

struct Edge {
  double cap;
  double flow;
  int vertex;
  int reverse;
};

struct Dinic {
  std::vector<std::vector<Edge>> graph_;

  std::vector<int> dist_;
  std::vector<int> current_;
  int source_;
  int sink_;
  int vertex_num_;
  const double eps = 1e-6;

  void AddEdge(int u, int v, double cap) {
    int reverse_u = graph_.at(v).size();
    int reverse_v = graph_.at(u).size();
    graph_[u].push_back(Edge{cap, 0, v, reverse_u});
    graph_[v].push_back(Edge{0, 0, u, reverse_v});
  }

  void Init(int vertex_num, int source, int sink) {
    source_ = source;
    sink_ = sink;
    vertex_num_ = vertex_num;
    graph_.resize(vertex_num);
    dist_.resize(vertex_num);
    current_.resize(vertex_num);
  }

  bool Bfs() {
    std::queue<int> que;
    que.push(source_);
    for (int &dis : dist_) {
      dis = -1;
    }
    dist_[source_] = 0;

    while (!que.empty()) {
      int u = que.front();
      que.pop();
      for (int i = 0; i < graph_.at(u).size(); ++i) {
        const auto &edge = graph_.at(u).at(i);
        int v = edge.vertex;
        if (edge.cap > edge.flow && dist_[v] == -1) {
          dist_[v] = dist_[u] + 1;
          que.push(v);
        }
      }
    }
    return dist_[sink_] != -1;
  }

  double Dfs(int u, double max_flow) {
    if (u == sink_ || fabs(max_flow) < eps) {
      return max_flow;
    }

    double flo;
    double ret = 0;

    for (auto &i = current_[u]; i < graph_.at(u).size(); ++i) {
      Edge &edge = graph_.at(u).at(i);
      int v = edge.vertex;
      Edge &reverse_edge = graph_.at(v).at(edge.reverse);
      if (dist_[v] == dist_[u] + 1 &&
          (flo = Dfs(v, std::min(max_flow, edge.cap - edge.flow))) > 0) {
        max_flow -= flo;
        edge.flow += flo;
        reverse_edge.flow -= flo;
        ret += flo;
        if (max_flow == 0)
          break;
      }
    }
    return ret;
  }

  double GetMaxFlow() {
    double ret = 0;
    while (Bfs()) {
      ret += Dfs(source_, inf);
    }
    return ret;
  }
};

class SubGraph {
public:
  SubGraph() = default;

  inline const auto &GetShardTasks() const { return shard_tasks_; }
  inline const auto &GetResourcePack() const { return resource_pack_; }
  inline auto GetWorkerID() const { return worker_id_; }
  inline void SetWorkerID(int worker_id) { worker_id_ = worker_id; }
  inline void SetIsNodeManager() { is_worker_ = false; }
  inline const auto &GetDataLocality() const { return data_locality_; }
  inline auto GetMemory() const { return memory_; }
  inline auto GetJobID() const { return job_id_; }
  void SetJobID(int job_id) {
    job_id_ = job_id;
    for (auto &task : shard_tasks_) {
      task.SetJobID(job_id);
    }
  }

  friend void from_json(const json &j, SubGraph &sg) {
    j.at("shardtask").get_to(sg.shard_tasks_);
    sg.SetResourcesReq();
    sg.memory_ = sg.GetMemoryCap();
  }

  double GetMemoryCap() {
    int source = 0;
    int sink = shard_tasks_.size() + 1;
    std::map<ShardTaskID, int> shard_task_id;
    for (int i = 0; i < shard_tasks_.size(); ++i) {
      const auto &st = shard_tasks_.at(i);
      shard_task_id[ShardTaskID{st.GetTaskID(), st.GetShardID()}] = i + 1;
    }

    Dinic dinic;
    dinic.Init(sink + 1, sink, source);
    int result = 0;
    // add edge : u -> v
    for (const auto &st : shard_tasks_) {
      int u = shard_task_id[ShardTaskID{st.GetTaskID(), st.GetShardID()}];
      if (st.GetMemory() > 0) {
        result += st.GetMemory();
        dinic.AddEdge(source, u, st.GetMemory());
      } else {
        dinic.AddEdge(u, sink, -st.GetMemory());
      }

      for (const auto &child : st.GetChildren()) {
        int v = shard_task_id[child];
        dinic.AddEdge(v, u, inf);
      }
    }
    result -= dinic.GetMaxFlow();
    return result;
  }

  void SetResourcesReq() {
    if (is_worker_) {
      for (auto &st : shard_tasks_) {
        if (st.GetResourceType() == ResourceType::kCPU) {
          resource_pack_.SetCPU(resource_pack_.GetCPU() + st.GetReq());
        } else if (st.GetResourceType() == ResourceType::kDisk) {
          resource_pack_.SetDisk(resource_pack_.GetDisk() + st.GetReq());
        } else if (st.GetResourceType() == ResourceType::kNetwork) {
          resource_pack_.SetNetwork(resource_pack_.GetNetwork() + st.GetReq());
        }
      }
      resource_pack_.SetMemory(GetMemoryCap());
    } else {
      for (auto &st : shard_tasks_) {
        if (st.GetResourceType() == ResourceType::kCPU) {
          resource_pack_.SetCPU(resource_pack_.GetCPU() + 1);
        }
      }
      resource_pack_.SetDisk(0);
      resource_pack_.SetNetwork(0);
      resource_pack_.SetMemory(GetMemoryCap());
    }
  }

  void Print() {
    DLOG(INFO) << "worker id : " << worker_id_;
    resource_pack_.Print();
    for (auto &st : shard_tasks_) {
      st.Print();
    }
  }

private:
  int memory_;
  int job_id_;
  bool is_worker_ = true;
  ResourcePack resource_pack_;
  std::vector<ShardTask> shard_tasks_;
  std::vector<int> data_locality_;
  int worker_id_ = -1;
};

} // namespace simulation
} // namespace axe
