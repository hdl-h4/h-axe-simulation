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

#include <chrono>
#include <ctime>
#include <fstream>
#include <string>

#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include "common.h"
#include "play_back/player.h"
#include "simulator.h"
#include "worker/worker.h"

/*
using json = nlohmann::json;

json ReadJsonFromFile(const std::string &file) {
  auto read_start = std::chrono::system_clock::now();
  std::ifstream in(file, std::ios::in);
  CHECK(in.is_open()) << "Cannot open json file " << file;
  std::string ret;
  in.seekg(0, std::ios::end);
  ret.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&ret[0], ret.size());
  in.close();
  auto read_end = std::chrono::system_clock::now();
  std::cout << "Reading " << file << " uses time : "
            << std::chrono::duration<double>(read_end - read_start).count()
            << std::endl;
  return json::parse(ret);
}

void PrintAlgorithm() {
  DLOG(INFO) << "Algorithm:";
  DLOG(INFO) << "TaskPlacement: "
             << axe::simulation::AlgorithmBook::GetTaskPlacementString();
}

void SetAlgorithm(const json &j) {
  DLOG(INFO) << "set alg";
  std::string task_placement_str;
  j.at("TaskPlacement").get_to(task_placement_str);
  axe::simulation::AlgorithmBook::Init(task_placement_str);
  PrintAlgorithm();
}

*/

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetLogDestination(google::INFO, "log/SIMULATION.INFO");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logbuflevel = -1;

  std::string mode(argv[1]);

  if (mode == "PLAYER") {
    LOG(INFO) << "start play";
    axe::simulation::Player player;

    player.Init(axe::simulation::ReadJsonFromFile(argv[2]));
    LOG(INFO) << "play end";
  } else {
    LOG(INFO) << "start simulation";
    auto init_start = std::chrono::system_clock::now();
    axe::simulation::Simulator simulator(
        axe::simulation::ReadJsonFromFile(argv[2]),
        axe::simulation::ReadJsonFromFile(argv[3]), argv[5]);
    axe::simulation::SetAlgorithm(axe::simulation::ReadJsonFromFile(argv[4]));
    auto init_end = std::chrono::system_clock::now();
    std::cout << "Init uses time : "
              << std::chrono::duration<double>(init_end - init_start).count()
              << std::endl;

    auto init2_start = std::chrono::system_clock::now();
    simulator.Init(argv[5]);
    auto init2_end = std::chrono::system_clock::now();
    std::cout << "Init2 uses time : "
              << std::chrono::duration<double>(init2_end - init2_start).count()
              << std::endl;

    /*
    auto print_start = std::chrono::system_clock::now();
    simulator.Print();
    auto print_end = std::chrono::system_clock::now();
    std::cout << "Print uses time : " << std::chrono::duration<double>(print_end
    -
    print_start).count() << std::endl;
    */

    auto serve_start = std::chrono::system_clock::now();
    simulator.Serve();
    auto serve_end = std::chrono::system_clock::now();
    std::cout << "Serving use time : "
              << std::chrono::duration<double>(serve_end - serve_start).count()
              << std::endl;
    auto report_start = std::chrono::system_clock::now();
    simulator.Report();
    auto report_end = std::chrono::system_clock::now();
    std::cout
        << "Reportation use time : "
        << std::chrono::duration<double>(report_end - report_start).count()
        << std::endl;
    LOG(INFO) << "simulation end";
  }
  google::FlushLogFiles(google::INFO);
  gflags::ShutDownCommandLineFlags();
  return 0;
}
