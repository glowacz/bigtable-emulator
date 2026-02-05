#include "storage.h"
#include "constants.h"
#include "rocksdb/iterator.h"
#include <iostream>
#include <vector>
#include <sstream>

Storage::Storage(const std::string& db_path) {
    rocksdb::Options options;
    options.create_if_missing = true; 

    // 1. List existing column families
    std::vector<std::string> family_names;
    rocksdb::Status status = rocksdb::DB::ListColumnFamilies(options, db_path, &family_names);
    
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;

    // If DB exists but ListColumnFamilies failed (unlikely) or returned empty, 
    // we must at least open the default.
    if (!status.ok() || family_names.empty()) {
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
    } else {
        for (const auto& name : family_names) {
            column_families.push_back(rocksdb::ColumnFamilyDescriptor(name, rocksdb::ColumnFamilyOptions()));
        }
    }

    // 2. Open DB with all column families
    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::DB* db_ptr = nullptr;
    status = rocksdb::DB::Open(options, db_path, column_families, &handles, &db_ptr);

    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
        // In a real app, throw or exit.
    }
    else {
        std::cout << "Opened RocksDB with " << handles.size() << " column families.\n";
        
        // 3. Map handles to names
        for (size_t i = 0; i < column_families.size(); i++) {
            cf_handles_[column_families[i].name] = handles[i];
        }
    }
    db_.reset(db_ptr);
}

Storage::~Storage() {
    // ColumnFamilyHandles must be deleted before the DB is deleted.
    for (auto& pair : cf_handles_) {
        if (pair.second) {
            db_->DestroyColumnFamilyHandle(pair.second);
        }
    }
    cf_handles_.clear();
    // db_ unique_ptr will close the DB automatically here
}

rocksdb::ColumnFamilyHandle* Storage::GetOrAddHandle(const std::string& cf_name) {
    std::lock_guard<std::mutex> lock(cf_mutex_);
    
    // Check if it exists
    auto it = cf_handles_.find(cf_name);
    if (it != cf_handles_.end()) {
        return it->second;
    }

    // Create new column family
    rocksdb::ColumnFamilyHandle* handle;
    rocksdb::Status status = db_->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), cf_name, &handle);
    
    if (!status.ok()) {
        std::cerr << "Failed to create Column Family '" << cf_name << "': " << status.ToString() << std::endl;
        return nullptr;
    }

    cf_handles_[cf_name] = handle;
    std::cout << "Created new Column Family: " << cf_name << "\n";
    return handle;
}

bool Storage::PutCell(const std::string& table_name, const std::string& row_key, const std::string& column_family,
      const std::string& column_qualifier, const std::chrono::milliseconds& timestamp, 
      const std::string& value) {
    
    rocksdb::ColumnFamilyHandle* handle = GetOrAddHandle(column_family);
    if (!handle) return false;

    // Note: column_family name is removed from the key string because 
    // it is now represented by the physical RocksDB Column Family.
    std::string full_key = "/tables/" + table_name + "/" + row_key + "/" + column_qualifier + "/" + std::to_string(timestamp.count());
    
    // Debug print
    std::cout << "PutCell [" << column_family << "]: " << full_key << " = " << value << "\n";

    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), handle, full_key, value);
    return status.ok();
}

bool Storage::PutRow(const std::string& row_key, const std::string& value) {
    // Uses Default CF
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), row_key, value);
    return status.ok();
}

std::string Storage::GetRow(const std::string& row_key) {
    // Uses Default CF
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), row_key, &value);
    if (!status.ok()) {
        return ""; 
    }
    return value;
}

bool Storage::DeleteRow(const std::string& row_key) {
    // Uses Default CF
    rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), row_key);
    return status.ok();
}

bool Storage::PutBatch(const std::vector<std::pair<std::string,std::string>>& kvs) {
    // Writes to Default CF
    rocksdb::WriteBatch batch;
    for (auto const& kv : kvs) {
        batch.Put(kv.first, kv.second);
    }
    rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!status.ok()) {
        std::cerr << "PutBatch failed: " << status.ToString() << std::endl;
        return false;
    }
    return true;
}

void Storage::ScanDatabase(void) {
    std::lock_guard<std::mutex> lock(cf_mutex_);
    rocksdb::ReadOptions read_options;

    std::cout << "--- Scanning Database ---\n";
    for (const auto& pair : cf_handles_) {
        std::string cf_name = pair.first;
        rocksdb::ColumnFamilyHandle* handle = pair.second;

        std::cout << "Column Family: [" << cf_name << "]\n";
        
        rocksdb::Iterator* it = db_->NewIterator(read_options, handle);
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            std::cout << "  " << it->key().ToString() << " => " << it->value().ToString() << "\n";
        }
        if (!it->status().ok()) {
            std::cerr << "  Error: " << it->status().ToString() << "\n";
        }
        delete it;
    }
    std::cout << "-------------------------\n";
}

void Storage::GetRowData(const std::string& table_name, const std::string& row_key) {
    // Since data is split across column families, we must check all of them 
    // for this specific row key prefix.
    
    std::string prefix = "/tables/" + table_name + "/" + row_key + "/";
    rocksdb::ReadOptions read_options;
    
    std::lock_guard<std::mutex> lock(cf_mutex_);

    for (const auto& pair : cf_handles_) {
        rocksdb::ColumnFamilyHandle* handle = pair.second;
        rocksdb::Iterator* it = db_->NewIterator(read_options, handle);

        for (it->Seek(prefix); it->Valid(); it->Next()) {
            if (!it->key().starts_with(prefix)) {
                break;
            }
            // Output format: CF:Key => Value
            std::cout << "[" << pair.first << "] " << it->key().ToString() << " | Value: " << it->value().ToString() << std::endl;
        }
        delete it;
    }
}

void Storage::DeleteTable(std::string table_key) {
    std::string table_key_to_remove = kTablesPrefix + table_key;
    std::string manifest = GetRow(kManifestKey);

    std::string new_manifest;
    bool changed = false;

    if (!manifest.empty()) {
        std::istringstream iss(manifest);
        std::string line;

        // Iterate through lines, keeping only those that do not match the key
        while (std::getline(iss, line)) {
            if (Trim(line) != table_key_to_remove) {
            new_manifest += line;
            new_manifest.push_back('\n'); // Maintain newline format
            } else {
            changed = true; // We found and skipped the target key
            }
        }
    }

    // std::cout << "Old manifest was: " << manifest << "\n";
    // std::cout << "New manifest is: " << new_manifest << "\n";

    if (changed) {
        // Save manifest with this table deleted
        PutRow(kManifestKey, new_manifest);

        // delete column families
        DeleteColumnFamiliesForTable(table_key + "/");

        std::string start_key = "/tables/" + table_key + "/";

        // // Delete cell values - now deleting column families handles that

        // std::cout << "Deleting all cells with prefix " << start_key << "\n";
        
        // std::string end_key = CalculatePrefixEnd(start_key);

        // rocksdb::WriteBatch batch;
        
        // for (const auto& pair : cf_handles_) {
        //     rocksdb::ColumnFamilyHandle* handle = pair.second;
        //     std::cout << "===================================\nDeleting all cells for column family " << pair.first << "\n===================================\n";
        //     batch.DeleteRange(handle, start_key, end_key);
        // }
        
        // batch.DeleteRange(db_->DefaultColumnFamily(), start_key, end_key);

        // rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch);

        // Delete the persisted schema
        std::cout << "Deleting the persisted schema under key: " << start_key << "\n";
        DeleteRow(table_key_to_remove);

        // // return status.ok();
    }
}

void Storage::DeleteColumnFamiliesForTable(const std::string& table_prefix) {
    std::lock_guard<std::mutex> lock(cf_mutex_);
    std::vector<std::string> cfs_to_remove;

    // 1. Identify Column Families that match the prefix
    for (const auto& pair : cf_handles_) {
        const std::string& name = pair.first;
        
        // Safety: Never drop the default column family
        if (name == rocksdb::kDefaultColumnFamilyName) {
            continue;
        }

        // Check if name starts with prefix
        if (name.size() >= table_prefix.size() && 
            name.compare(0, table_prefix.size(), table_prefix) == 0) {
            cfs_to_remove.push_back(name);
        }
    }

    // 2. Drop and Destroy handles
    for (const auto& name : cfs_to_remove) {
        DeleteColumnFamily(name);
    }
}

void Storage::DeleteColumnFamily(const std::string &prefixed_cf_name) {
    rocksdb::ColumnFamilyHandle* handle = cf_handles_[prefixed_cf_name];
        
    std::cout << "Dropping Column Family: " << prefixed_cf_name << "\n";
    
    // Drop the Column Family from RocksDB (persisted deletion)
    rocksdb::Status status = db_->DropColumnFamily(handle);
    if (!status.ok()) {
        std::cerr << "Failed to drop Column Family '" << prefixed_cf_name << "': " << status.ToString() << "\n";
        return;
    }

    // Destroy the in-memory handle object
    status = db_->DestroyColumnFamilyHandle(handle);
    if (!status.ok()) {
        std::cerr << "Failed to destroy handle for '" << prefixed_cf_name << "': " << status.ToString() << "\n";
    }

    // Remove from our internal map
    cf_handles_.erase(prefixed_cf_name);
}

rocksdb::Iterator* Storage::NewIterator(const std::string& cf_name) {
    rocksdb::ColumnFamilyHandle* handle = GetOrAddHandle(cf_name);
    if (!handle) return nullptr;
    return db_->NewIterator(rocksdb::ReadOptions(), handle);
}

static Storage *g_storage = NULL;
static pthread_once_t g_storage_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_storage_mu = PTHREAD_MUTEX_INITIALIZER;
static char *g_storage_db_name = NULL;
std::atomic<int> idx{0};

static void init_storage_once(void) {
  if (g_storage_db_name == NULL) {
    return;
  }
  g_storage = new Storage(g_storage_db_name);
}

int InitGlobalStorage(const char *db_name) {
  if (db_name == NULL) return -1;
  pthread_mutex_lock(&g_storage_mu);
  if (g_storage != NULL) {
    // already initialized
    pthread_mutex_unlock(&g_storage_mu);
    return 0;
  }

  g_storage_db_name = strdup(db_name);
  if (g_storage_db_name == NULL) {
    pthread_mutex_unlock(&g_storage_mu);
    return -1;
  }

  int rc = pthread_once(&g_storage_once, init_storage_once);
  pthread_mutex_unlock(&g_storage_mu);
  return rc;
}

Storage *GetGlobalStorage(void) {
  return g_storage;
}

void CloseGlobalStorage(void) {
  pthread_mutex_lock(&g_storage_mu);
  if (g_storage) {
    delete g_storage;
    g_storage = NULL;
  }
  if (g_storage_db_name) {
    free(g_storage_db_name);
    g_storage_db_name = NULL;
  }
  pthread_mutex_unlock(&g_storage_mu);
}

int GetNextSchemaIdx()
{
    return idx++;
}

void RollbackSchemaIdx()
{
    idx--;
}

std::string Trim(const std::string& s) {
  size_t a = 0;
  while (a < s.size() && (s[a] == '\n' || s[a] == '\r')) ++a;
  size_t b = s.size();
  while (b > a && (s[b-1] == '\n' || s[b-1] == '\r')) --b;
  return s.substr(a, b - a);
}

// Helper to calculate the smallest string strictly greater than all keys starting with 'prefix'
std::string CalculatePrefixEnd(const std::string& prefix) {
    std::string end_key = prefix;
    // Strip trailing 0xFF bytes as they cannot be incremented
    while (!end_key.empty() && static_cast<unsigned char>(end_key.back()) == 0xFF) {
        end_key.pop_back();
    }
    
    if (end_key.empty()) {
        // Corner case: The prefix was all 0xFF. Theoretically, there is no end key.
        // In your specific schema ("/tables/..."), this will never happen.
        return "\xff"; 
    }
    
    // Increment the last byte to get the next prefix
    end_key.back()++;
    return end_key;
}