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
    bool DeleteRow(const std::string& row_key);

    // Atomic write of several key/value pairs.
    bool PutBatch(const std::vector<std::pair<std::string, std::string>>& kvs);

private:
    std::unique_ptr<rocksdb::DB> db_;
};

// There will be only one global storage, so we expose API to use it

// invoked in main only
int InitGlobalStorage(const char *path);
// Returns the Storage pointer or NULL if not initialized
Storage *GetGlobalStorage(void);
// Close / free resources
void CloseGlobalStorage(void);

int GetNextSchemaIdx();
void RollbackSchemaIdx();

// helper function, this should not be here, to be moved to different header in future 
std::string Trim(const std::string& s);