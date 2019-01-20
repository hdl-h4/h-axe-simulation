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

#include "algorithm/algorithm_book.h"
#include "glog/logging.h"
#include "resource/resource_request.h"
#include "scheduler.h"
#include "worker.h"
#include "gtest/gtest.h"
#include <vector>

namespace axe {
namespace simulation {

class TestScheduler : public testing::Test {
public:
  TestScheduler() {}
  ~TestScheduler() {}

protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(TestScheduler, NewJobEvent) {
  std::shared_ptr<std::vector<Worker>> workers =
      std::make_shared<std::vector<Worker>>();
  workers->push_back(Worker(5, 5, 5, 5));
  workers->push_back(Worker(5, 5, 5, 5));
  std::shared_ptr<NewJobEvent> new_job_event =
      std::make_shared<NewJobEvent>(NEW_JOB, 10, 0, 0, Job());
  Scheduler scheduler(workers, std::shared_ptr<std::vector<User>>());
  std::vector<std::shared_ptr<Event>> ret_vector =
      scheduler.Handle(new_job_event);
  EXPECT_EQ(ret_vector.size(), 1);
  auto event = std::static_pointer_cast<JobAdmissionEvent>(ret_vector[0]);
  EXPECT_EQ(event->GetEventType(), JOB_ADMISSION);
  EXPECT_DOUBLE_EQ(event->GetTime(), 10);
  EXPECT_EQ(event->GetPriority(), 0);
  EXPECT_EQ(event->GetEventPrincipal(), 0);
}

/*
TEST_F(TestScheduler, NewTaskReqEvent) {
  axe::simulation::AlgorithmBook::Init("FIFO");
  std::shared_ptr<std::vector<Worker>> workers =
      std::make_shared<std::vector<Worker>>();
  workers->push_back(Worker(5, 5, 5, 5));
  workers->push_back(Worker(5, 5, 5, 5));
  ResourceRequest req(5, 6, std::vector<int>(0, 1), ResourcePack(1, 2, 3, 4));
  std::shared_ptr<NewTaskReqEvent> new_task_req_event =
      std::make_shared<NewTaskReqEvent>(NEW_TASK_REQ, 10, 0, 5, req);
  Scheduler scheduler(workers, std::shared_ptr<std::vector<User>>());
  std::vector<std::shared_ptr<Event>> ret_vector =
      scheduler.Handle(new_task_req_event);
  EXPECT_EQ(ret_vector.size(), 1);
  auto event = std::static_pointer_cast<PlacementDecisionEvent>(ret_vector[0]);
  EXPECT_EQ(event->GetEventType(), PLACEMENT_DECISION);
  EXPECT_DOUBLE_EQ(event->GetTime(), 10);
  EXPECT_EQ(event->GetPriority(), 0);
  EXPECT_EQ(event->GetEventPrincipal(), 5);
  EXPECT_EQ(event->GetWorkerId(), 0);
  EXPECT_EQ(event->GetSubGraphId(), 6);
}
*/

} // namespace simulation
} // namespace axe
