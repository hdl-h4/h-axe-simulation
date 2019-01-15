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
  Scheduler scheduler(workers);
  std::vector<std::shared_ptr<Event>> ret_vector =
      scheduler.Handle(new_job_event);
  EXPECT_EQ(ret_vector.size(), 1);
  EXPECT_EQ(ret_vector[0]->GetEventType(), JOB_ADMISSION);
  EXPECT_EQ(ret_vector[0]->GetTime(), 10);
  EXPECT_EQ(ret_vector[0]->GetEventPrincipal(), 0);
}

} // namespace simulation
} // namespace axe
