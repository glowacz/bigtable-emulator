// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_cat.h"
#include "server.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include "storage_engine.h"

ABSL_FLAG(std::string, host, "localhost",
          "the address to bind to on the local machine");
ABSL_FLAG(std::uint16_t, port, 8888,
          "the port to bind to on the local machine");

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(
      absl::StrCat("Usage: %s -h <host> -p <port>", argv[0]));
  absl::ParseCommandLine(argc, argv);

  auto maybe_server =
      google::cloud::bigtable::emulator::CreateDefaultEmulatorServer(
          absl::GetFlag(FLAGS_host), absl::GetFlag(FLAGS_port));
  if (!maybe_server) {
    std::cerr << "CreateDefaultEmulatorServer() failed. See logs for "
                 "possible reason"
              << std::endl;
    return 1;
  }

  // --- START OF TEST CODE ---
  std::cout << "--- Starting RocksDB Test ---" << std::endl;
  
  // 1. Initialize the engine (creates ./test_db folder)
  StorageEngine engine("test_db");

  // 2. Write a test row
  std::string key = "test-row-key";
  std::string value = "Hello, RocksDB!";
  if (engine.PutRow(key, value)) {
      std::cout << "Successfully wrote row: " << key << std::endl;
  } else {
      std::cerr << "Failed to write row!" << std::endl;
  }

  // 3. Read the row back
  std::string result = engine.GetRow(key);
  std::cout << "Read back value: " << result << std::endl;
  std::cout << "--- End of RocksDB Test ---" << std::endl;
  // --- END OF TEST CODE ---

  auto& server = maybe_server.value();

  std::cout << "Server running on port " << server->bound_port() << "\n";
  server->Wait();
  return 0;
}
