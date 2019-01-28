[![wercker status](https://app.wercker.com/status/fdaf3c83d0a4cbaa37d6bdd18b547df1/s/master "wercker status")](https://app.wercker.com/project/byKey/fdaf3c83d0a4cbaa37d6bdd18b547df1)
# h-axe-simulation

Simulation project for various scheduling algorithm.

### Build

1. Build all targets(in release mode for example)

        $ mkdir build
        $ cd build
        $ cmake -DCMAKE_BUILD_TYPE=Release ..  # CMAKE_BUILD_TYPE: Release, Debug, RelWithDebInfo
        $ make -j{N}

2. Run Unittest

        $ make test                            # Run unit test


### Configuration

There are three types of configuration files in conf/ directory: worker file, job file and algorithm file.

### Run Simulation

1. Run simulation with configuration files

        $ cd build
        $ ./Simulation ../conf/workers.json ../conf/jobs.json  ../conf/alg.json
