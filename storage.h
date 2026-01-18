#pragma once
#include <string>
#include <memory>
#include "rocksdb/db.h"

class Storage {
public:
    Storage(const std::string& db_path);
    ~Storage();

    // Basic abstraction for Bigtable "MutateRow"
    bool PutRow(const std::string& row_key, const std::string& value);
    std::string GetRow(const std::string& row_key);

private:
    std::unique_ptr<rocksdb::DB> db_;
};