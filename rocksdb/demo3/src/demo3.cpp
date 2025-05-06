#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>

#include <cassert>
#include <ctime>
#include <iostream>

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::Transaction;
using ROCKSDB_NAMESPACE::TransactionDB;
using ROCKSDB_NAMESPACE::TransactionDBOptions;
using ROCKSDB_NAMESPACE::TransactionOptions;
using ROCKSDB_NAMESPACE::WriteOptions;

int main() {
    std::string db_path = "/tmp/rocksdb-demo3";

    Options options;
    options.create_if_missing = true;

    TransactionDB *txn_db;
    TransactionDBOptions txn_db_options;
    Status s = TransactionDB::Open(options, txn_db_options, db_path, &txn_db);
    assert(s.ok());

    WriteOptions write_options;
    ReadOptions read_options;
    TransactionOptions txn_options;
    //////////////////////////////////////////////////////////////
    // Simple Transaction Example ("Read Commited")
    //////////////////////////////////////////////////////////////
    Transaction *txn = txn_db->BeginTransaction(write_options);
    assert(txn);

    std::string value;
    s = txn->Get(read_options, "abc", &value);
    assert(s.IsNotFound());

    s = txn->Put("abc", "def");
    assert(s.ok());

    // Read OUTSIDE transaction, do not affect txn.
    s = txn_db->Get(read_options, "abc", &value);
    assert(s.IsNotFound());
    // Write a key OUTSIDE of this transaction.
    s = txn_db->Put(write_options, "xyz", "zzz");
    assert(s.ok());
    // Write OUTSIDE transaction will fail if key have written in txn
    s = txn_db->Put(write_options, "abc", "ddd");
    assert(s.subcode() == Status::kLockTimeout);

    // Value for key 'xyz' has been committed, can be read in txn.
    s = txn->Get(read_options, "xyz", &value);
    assert(s.ok());
    assert(value == "zzz");

    // Commit transaction.
    s = txn->Commit();
    assert(s.ok());
    delete txn;

    // Value is committed, can be read now.
    s = txn_db->Get(read_options, "abc", &value);
    assert(s.ok());
    assert(value == "def");

    //////////////////////////////////////////////////////////////
    // "Repeatable Read" (snapshot isolation) Example
    //////////////////////////////////////////////////////////////
    txn_options.set_snapshot = true;
    txn = txn_db->BeginTransaction(write_options, txn_options);

    const rocksdb::Snapshot *snapshot = txn->GetSnapshot();
    assert(s.ok());

    // Write a key OUTSIDE of transaction.
    s = txn_db->Put(write_options, "abc", "xyz");
    assert(s.ok());

    // Read the latest committed value.
    s = txn->Get(read_options, "abc", &value);
    assert(s.ok());
    assert(value == "xyz");

    // Read the snapshotted value.
    read_options.snapshot = snapshot;
    s = txn->Get(read_options, "abc", &value);
    assert(s.ok());
    assert(value == "def");

    // Attemp to read a key using the snapshot. This will fail since the
    // previous write outside this txn conflicts with this read.
    s = txn->GetForUpdate(read_options, "abc", &value);
    assert(s.IsBusy());

    txn->Rollback();
    // Snapshot will be released upon deleting the transaction.
    delete txn;

    // Clear snapshot from read options since it is no longer valid
    snapshot = nullptr;
    read_options.snapshot = nullptr;

    delete txn_db;
    return 0;
}
