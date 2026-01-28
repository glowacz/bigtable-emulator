// #include "absl/flags/flag.h"
// #include "absl/flags/parse.h"
// #include "absl/flags/usage.h"
// #include "absl/strings/str_cat.h"
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include "storage.h"

// ABSL_FLAG(std::string, host, "localhost",
//           "the address to bind to on the local machine");
// ABSL_FLAG(std::uint16_t, port, 8888,
//           "the port to bind to on the local machine");

int main(int argc, char* argv[]) {
  // absl::ParseCommandLine(argc, argv);
  if (const char* env_p = std::getenv("BUILD_WORKING_DIRECTORY")) {
    if (chdir(env_p) != 0) {
        std::cerr << "Failed to switch to workspace directory.\n";
    }
  }

  std::string db_path = "test_db";
  rocksdb::Options options;
  rocksdb::Status status = rocksdb::DestroyDB(db_path, options);
  std::cout << "Status of destroying DB: " << status.ok() << "\n";
  return 0;
}
