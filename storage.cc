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