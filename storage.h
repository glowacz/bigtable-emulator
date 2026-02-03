#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "rocksdb/db.h"

class Storage {
public:
    Storage(const std::string& db_path);
    ~Storage();

    bool PutCell(const std::string& table_name, const std::string& row_key, const std::string& column_family,
        const std::string& column_qualifier, const std::chrono::milliseconds& timestamp, 
        const std::string& value);
        
    // Basic abstraction for Bigtable "MutateRow" (Uses Default Column Family)
    bool PutRow(const std::string& row_key, const std::string& value);
    std::string GetRow(const std::string& row_key);
    bool DeleteRow(const std::string& row_key);

    // Atomic write of several key/value pairs (Uses Default Column Family)
    bool PutBatch(const std::vector<std::pair<std::string, std::string>>& kvs);
    
    // Scans all Column Families and prints data
    void ScanDatabase(void);
    
    // Scans all Column Families for a specific row key
    void GetRowData(const std::string& table_name, const std::string& row_key);

    // Deletes the table from the manifest and deletes table contents
    void DeleteTable(std::string table_name);

    // Returns an iterator for a specific column family
    rocksdb::Iterator* NewIterator(const std::string& cf_name);

private:
    std::unique_ptr<rocksdb::DB> db_;
    
    // Map to store handles for each Column Family
    std::unordered_map<std::string, rocksdb::ColumnFamilyHandle*> cf_handles_;
    std::mutex cf_mutex_;

    // Helper to retrieve or create a handle
    rocksdb::ColumnFamilyHandle* GetOrAddHandle(const std::string& cf_name);
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

// Helper to calculate the smallest string strictly greater than all keys starting with 'prefix'
std::string CalculatePrefixEnd(const std::string& prefix);