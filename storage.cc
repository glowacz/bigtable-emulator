#include "storage.h"
#include <iostream>

Storage::Storage(const std::string& db_path) {
    rocksdb::Options options;
    // Essential for an emulator: create the DB file if it doesn't exist
    options.create_if_missing = true; 

    rocksdb::DB* db_ptr = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_ptr);

    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
        // In a real project, handle error or throw exception
    }
    db_.reset(db_ptr);
}

Storage::~Storage() {
    // db_ is a unique_ptr, so it will close automatically
}

bool Storage::PutRow(const std::string& row_key, const std::string& value) {
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), row_key, value);
    return status.ok();
}

std::string Storage::GetRow(const std::string& row_key) {
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), row_key, &value);
    if (!status.ok()) {
        return ""; // Or handle "Not Found" specifically
    }
    return value;
}

bool Storage::DeleteRow(const std::string& row_key) {
    rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), row_key);
    return status.ok();
}

bool Storage::PutBatch(const std::vector<std::pair<std::string,std::string>>& kvs) {
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

  // copy db_name into owned memory (so init_storage_once can use it safely)
  g_storage_db_name = strdup(db_name);
  if (g_storage_db_name == NULL) {
    pthread_mutex_unlock(&g_storage_mu);
    return -1;
  }

  // initialize the actual storage in a threadsafe once
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
