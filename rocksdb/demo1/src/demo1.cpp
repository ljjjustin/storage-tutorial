#include <rocksdb/db.h>

#include <cassert>
#include <ctime>

int main() {
    rocksdb::DB *db;
    rocksdb::Options options;
    options.create_if_missing = true;

    std::string db_path = "/tmp/rocksdb-demo1";
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
    assert(status.ok());

    status = db->Put(rocksdb::WriteOptions(), "key1", "value1");
    assert(status.ok());

    std::string value;
    status = db->Get(rocksdb::ReadOptions(), "key1", &value);
    assert(status.ok());
    assert(value == "value1");

    // apply a set of updates
    {
        rocksdb::WriteBatch batch;
        batch.Delete("key1");
        batch.Put("key2", "value2");
        status = db->Write(rocksdb::WriteOptions(), &batch);
        assert(status.ok());
    }

    status = db->Get(rocksdb::ReadOptions(), "key1", &value);
    assert(status.IsNotFound());

    std::string value2;
    status = db->Get(rocksdb::ReadOptions(), "key2", &value2);
    assert(status.ok());
    assert(value2 == "value2");

    delete db;
    return 0;
}
