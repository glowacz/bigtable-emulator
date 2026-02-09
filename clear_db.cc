#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include "storage.h"

int main() {
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
