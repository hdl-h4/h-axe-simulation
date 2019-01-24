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

#include "scheduler/scheduler_impl.h"

#include "scheduler/simple_scheduler_impl.h"

namespace axe {
namespace simulation {

std::shared_ptr<SchedulerImpl>
SchedulerImpl::CreateImpl(const std::shared_ptr<std::vector<Worker>> &workers,
                          const std::shared_ptr<std::vector<User>> &users,
                          const std::string &mode) {
  return std::make_shared<SimpleSchedulerImpl>(workers, users);
}

} // namespace simulation
} // namespace axe
