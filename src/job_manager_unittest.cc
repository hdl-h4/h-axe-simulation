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

#include <fstream>
#include <vector>

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "algorithm/algorithm_book.h"
#include "job_manager.h"
#include "resource/resource_request.h"
#include "worker/worker.h"

namespace axe {
namespace simulation {

class TestJobManager : public testing::Test {
public:
  TestJobManager() {}
  ~TestJobManager() {}

protected:
  void SetUp() {}
  void TearDown() {}
};
/*
json ReadJsonFromFile(const std::string &file) {
  std::ifstream in(file, std::ios::in);
  CHECK(in.is_open()) << "Cannot open json file " << file;
  std::string ret;
  in.seekg(0, std::ios::end);
  ret.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&ret[0], ret.size());
  return json::parse(ret);
}

TEST_F(TestJobManager, JobAdmissionEvent) {
  std::vector<Job> jobs;
  json j = ReadJsonFromFile("../../conf/jobs.json");
  j.at("job").get_to(jobs);
  std::vector<std::shared_ptr<JobManager>> jms;
  std::shared_ptr<std::vector<Worker>> workers =
      std::make_shared<std::vector<Worker>>();
  workers->push_back(Worker(5, 5, 5, 5));
  workers->push_back(Worker(5, 5, 5, 5));
  std::shared_ptr<std::vector<User>> users =
      std::make_shared<std::vector<User>>();

  ResourcePack cluster_resource_capacity(10, 10, 10, 10);
  int users_size = 0;
  for (const auto &job : jobs) {
    jms.push_back(std::make_shared<JobManager>(job, workers, users));
    users_size = std::max(users_size, job.GetUserID() + 1);
  }

  users->resize(users_size);
  for (const auto &job : jobs) {
    users->at(job.GetUserID()).AddJobID(job.GetJobID());
  }
  for (auto &user : *users) {
    user.SetClusterResourceCapacity(cluster_resource_capacity);
  }

  EXPECT_EQ(jms.size(), 1);
  std::shared_ptr<JobAdmissionEvent> job_admission_event =
      std::make_shared<JobAdmissionEvent>(EventType::JOB_ADMISSION, 10, 0, 0,
                                          0);
  std::vector<std::shared_ptr<Event>> ret_vector =
      jms[0]->Handle(job_admission_event);
  EXPECT_EQ(ret_vector.size(), 4);
  for (auto event : ret_vector) {
    EXPECT_EQ(event->GetEventType(), EventType::NEW_TASK_REQ);
    EXPECT_EQ(event->GetTime(), 10);
    EXPECT_EQ(event->GetPriority(), 0);
    EXPECT_EQ(event->GetEventPrincipal(), kScheduler);
  }
}
*/

} // namespace simulation
} // namespace axe
