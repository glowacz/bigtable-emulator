#include "storage.h"
#include "rocksdb/iterator.h"
#include <iostream>

Storage::Storage(const std::string& db_path) {
    rocksdb::Options options;
    options.create_if_missing = true; 

    rocksdb::DB* db_ptr = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_ptr);

    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
        // Handle error (throw exception ???)
    }
    else {
      std::cout << "Opened RocksDB\n";
    }
    db_.reset(db_ptr);
}

Storage::~Storage() {
    // db_ is a unique_ptr, so it will close automatically
}

bool Storage::PutCell(const std::string& table_name, const std::string& row_key, const std::string& column_family,
      const std::string& column_qualifier, const std::chrono::milliseconds& timestamp, 
      const std::string& value) {
    std::cout << "PutCell: before creating full key\n";
    std::string full_key = "/tables/" + table_name + "/" + row_key + "/" + column_family + "/" + column_qualifier + "/" + std::to_string(timestamp.count());
    std::cout << "Trying to put " << full_key << ": " << value << " into the DB\n";
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), full_key, value);
    std::cout << "Put cell into the DB with status " << status.ok() << "\n";
    return status.ok();
}

bool Storage::PutRow(const std::string& row_key, const std::string& value) {
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), row_key, value);
    return status.ok();
}

std::string Storage::GetRow(const std::string& row_key) {
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), row_key, &value);
    if (!status.ok()) {
        return ""; // Or handle "Not Found" specifically ???
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

void Storage::ScanDatabase(void) {
  rocksdb::ReadOptions read_options;
  rocksdb::Iterator* it = db_->NewIterator(read_options);

  for (it->SeekToFirst(); it->Valid(); it->Next()) {
      std::string key = it->key().ToString();
      std::string value = it->value().ToString();

      std::cout << "Key: " << key << " | Value: " << value << std::endl;
  }

  if (!it->status().ok()) {
      std::cerr << "An error occurred: " << it->status().ToString() << std::endl;
  }

  delete it;
}

struct RowRecord {
  std::string full_key;
  std::string value;
};

void Storage::GetRowData(const std::string& table_name, const std::string& row_key) {
  std::vector<RowRecord> results;
  
  std::string prefix = "/tables/" + table_name + "/" + row_key + "/";

  rocksdb::ReadOptions read_options;

  rocksdb::Iterator* it = db_->NewIterator(read_options);

  for (it->Seek(prefix); it->Valid(); it->Next()) {
      if (!it->key().starts_with(prefix)) {
          break;
      }

      std::string key = it->key().ToString();
      std::string value = it->value().ToString();
      std::cout << "Key: " << key << " | Value: " << value << std::endl;
      
      results.push_back({it->key().ToString(), it->value().ToString()});
  }

  if (!it->status().ok()) {
      std::cerr << "Error during scan: " << it->status().ToString() << std::endl;
  }

  delete it;
}

rocksdb::Iterator* Storage::NewIterator() {
  return db_->NewIterator(rocksdb::ReadOptions());
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
