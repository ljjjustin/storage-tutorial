#include <gflags/gflags.h>
#include <pthread.h>
#include <rocksdb/db.h>

#include <cassert>
#include <chrono>
#include <ctime>
#include <vector>

constexpr auto kIterations = 500'0000;

DEFINE_string(db, "/tmp/rocksdb-demo4", "rocksdb data path");
DEFINE_int32(threads, 2, "Read/Write concurrency.");
DEFINE_string(type, "write", "Benchmark type: read or write");

struct BenchArgs {
    rocksdb::DB *db;
    std::string type;
    int start_index;
};

void *bench_work(void *arg) {
    BenchArgs *ba = static_cast<BenchArgs *>(arg);
    assert(ba->db != NULL);
    assert(ba->start_index >= 0);

    auto read_options = rocksdb::ReadOptions();
    auto write_options = rocksdb::WriteOptions();

    if (ba->type == "read") {
        std::string value;
        for (auto i = 0; i < kIterations; i++) {
            std::string key =
                "/tmp/rocksdb-bench-" + std::to_string(ba->start_index + i);
            auto status = ba->db->Get(read_options, key, &value);
            assert(status.ok());
        }
    } else if (ba->type == "write") {
        for (auto i = 0; i < kIterations; i++) {
            std::string key =
                "/tmp/rocksdb-bench-" + std::to_string(ba->start_index + i);
            std::string value = "value" + std::to_string(ba->start_index + i);
            auto status = ba->db->Put(write_options, key, value);
            assert(status.ok());
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    rocksdb::DB *db;
    rocksdb::Options options;
    options.create_if_missing = true;

    rocksdb::Status status = rocksdb::DB::Open(options, FLAGS_db, &db);
    assert(status.ok());

    std::vector<BenchArgs> args;
    for (auto i = 0; i < FLAGS_threads; i++) {
        args.push_back(BenchArgs{db, FLAGS_type, i * 1'0000'0000});
    }

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<pthread_t> workers;
    for (auto &arg : args) {
        pthread_t thread;
        auto ret = pthread_create(&thread, NULL, bench_work, &arg);
        if (ret != 0) {
            printf("pthread_create failed: %d\n", ret);
            exit(1);
        }
        workers.push_back(thread);
    }

    for (auto &thread : workers) {
        pthread_join(thread, NULL);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printf("qps: %.2f\n", kIterations * FLAGS_threads * 1000.0 / duration.count());

    delete db;
    return 0;
}
