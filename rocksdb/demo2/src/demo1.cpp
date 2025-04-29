#include <rocksdb/db.h>

#include <cassert>
#include <ctime>
#include <iostream>

using ROCKSDB_NAMESPACE::ColumnFamilyDescriptor;
using ROCKSDB_NAMESPACE::ColumnFamilyHandle;
using ROCKSDB_NAMESPACE::ColumnFamilyOptions;
using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteOptions;

int main() {
    std::string db_path = "/tmp/rocksdb-cf-demo1";

    Options options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;

    DB *db;
    Status s;
    // s = DB::Open(options, db_path, &db);
    // assert(s.ok());

    //// create column family
    // ColumnFamilyHandle *cf;
    // s = db->CreateColumnFamily(ColumnFamilyOptions(), "new_cf", &cf);
    // assert(s.ok());

    //// close db
    // s = db->DestroyColumnFamilyHandle(cf);
    // assert(s.ok());
    // delete db;

    // open db with two column families
    std::vector<ColumnFamilyDescriptor> column_families;
    column_families.push_back(ColumnFamilyDescriptor(
        rocksdb::kDefaultColumnFamilyName, ColumnFamilyOptions()));
    column_families.push_back(
        ColumnFamilyDescriptor("new_cf", ColumnFamilyOptions()));

    std::vector<ColumnFamilyHandle *> handles;
    s = DB::Open(options, db_path, column_families, &handles, &db);
    assert(s.ok());

    for (auto h : handles) {
        s = db->Put(WriteOptions(), h, "key1", "value1");
        assert(s.ok());
    }

    // write to new_cf column family
    for (auto h : handles) {
        std::string value;
        s = db->Get(ReadOptions(), h, "key1", &value);
        assert(s.ok());
        std::cout << "[" << h->GetName() << "]"
                  << "key1 = " << value << std::endl;
    }

    for (auto h : handles) {
        s = db->Delete(rocksdb::WriteOptions(), h, "key1");
        assert(s.ok());
    }

    while (handles.empty()) {
        db->DestroyColumnFamilyHandle(handles.back());
        handles.pop_back();
    }

    delete db;
    return 0;
}
