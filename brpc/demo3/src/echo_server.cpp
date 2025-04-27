#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include "echo.pb.h"

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port, 18000, "TCP port of this server");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s`");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallels");

butil::atomic<int> nsd(0);

struct MySessionLocalData {
    MySessionLocalData() : x(123) {
        nsd.fetch_add(1, butil::memory_order_relaxed);
    }
    ~MySessionLocalData() {
        nsd.fetch_sub(1, butil::memory_order_relaxed);
    }

    int x;
};

class MySessionLocalDataFactory : public brpc::DataFactory {
public:
    void *CreateData() const {
        return new MySessionLocalData;
    }

    void DestroyData(void *d) const {
        delete static_cast<MySessionLocalData *>(d);
    }
};

butil::atomic<int> ntls(0);

struct MyThreadLocalData {
    MyThreadLocalData() : y(0) {
        ntls.fetch_add(1, butil::memory_order_relaxed);
    }

    ~MyThreadLocalData() {
        ntls.fetch_sub(1, butil::memory_order_relaxed);
    }

    static void deleter(void *d) {
        delete static_cast<MyThreadLocalData *>(d);
    }
    int y;
};

class MyThreadLocalDataFactory : public brpc::DataFactory {
public:
    void *CreateData() const {
        return new MyThreadLocalData;
    }

    void DestroyData(void *d) const {
        MyThreadLocalData::deleter(d);
    }
};

struct AsyncJob {
    MySessionLocalData *expected_session_local_data;
    int expected_session_value;
    brpc::Controller *ctrl;
    const demo3::EchoRequest *request;
    demo3::EchoResponse *response;
    google::protobuf::Closure *done;

    void run();

    void run_and_delete() {
        run();
        delete this;
    }
};

void AsyncJob::run() {
    brpc::ClosureGuard done_guard(done);

    // sleep some time to make sure that Echo() exits.
    bthread_usleep(10000);

    MySessionLocalData *sd =
        static_cast<MySessionLocalData *>(ctrl->session_local_data());
    CHECK_EQ(expected_session_local_data, sd);
    CHECK_EQ(expected_session_value, sd->x);

    response->set_message(request->message());
    if (FLAGS_echo_attachment) {
        ctrl->response_attachment().append(ctrl->request_attachment());
    }
}

static void *process_thread(void *args) {
    AsyncJob *job = static_cast<AsyncJob *>(args);
    job->run_and_delete();
    return NULL;
}

class EchoServiceWithLocalData : public demo3::EchoService {
public:
    EchoServiceWithLocalData() {
        CHECK_EQ(0, bthread_key_create(&_tls2_key, MyThreadLocalData::deleter));
    }
    ~EchoServiceWithLocalData() {
        CHECK_EQ(0, bthread_key_delete(_tls2_key));
    }

    void Echo(google::protobuf::RpcController *ctrl_base,
              const demo3::EchoRequest *request, demo3::EchoResponse *response,
              google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        brpc::Controller *ctrl = static_cast<brpc::Controller *>(ctrl_base);

        // session local data
        MySessionLocalData *sd =
            static_cast<MySessionLocalData *>(ctrl->session_local_data());
        if (sd == NULL) {
            ctrl->SetFailed(
                "Require ServerOptions.session_local_data_factory to be"
                " set with a correctly implemented instance");
            LOG(ERROR) << ctrl->ErrorText();
            return;
        }
        const int expected_value = sd->x + (((uintptr_t)ctrl) & 0XFFFFFFFF);
        sd->x = expected_value;

        // thread local data
        MyThreadLocalData *tls =
            static_cast<MyThreadLocalData *>(brpc::thread_local_data());
        if (tls == NULL) {
            ctrl->SetFailed(
                "Require ServerOptions.thread_local_data_factory "
                "to be set with a correctly implemented instance");
            LOG(ERROR) << ctrl->ErrorText();
            return;
        }
        tls->y = expected_value;

        // create bthread-local data for your own.
        MyThreadLocalData *tls2 =
            static_cast<MyThreadLocalData *>(bthread_getspecific(_tls2_key));
        if (tls2 == NULL) {
            tls2 = new MyThreadLocalData;
            CHECK_EQ(0, bthread_setspecific(_tls2_key, tls2));
        }
        tls2->y = expected_value + 1;

        // sleep a while to force context switching.
        bthread_usleep(10000);

        // tls is unchanged after context switching.
        CHECK_EQ(tls, brpc::thread_local_data());
        CHECK_EQ(expected_value, tls->y);

        CHECK_EQ(tls2, bthread_getspecific(_tls2_key));
        CHECK_EQ(expected_value + 1, tls2->y);

        // Process the request asynchronously.
        AsyncJob *job = new AsyncJob;
        job->expected_session_local_data = sd;
        job->expected_session_value = expected_value;
        job->ctrl = ctrl;
        job->request = request;
        job->response = response;
        job->done = done;

        bthread_t th;
        CHECK_EQ(0, bthread_start_background(&th, NULL, process_thread, job));

        // We don't want to call done->Run() here, release the guard.
        done_guard.release();

        LOG_EVERY_SECOND(INFO) << "ntls" << ntls.load(butil::memory_order_relaxed)
                               << " nsd=" << nsd.load(butil::memory_order_relaxed);
    }

private:
    bthread_key_t _tls2_key;
};

int main(int argc, char *argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    MySessionLocalDataFactory session_local_data_factory;

    MyThreadLocalDataFactory thread_local_data_factory;

    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.max_concurrency = FLAGS_max_concurrency;
    options.session_local_data_factory = &session_local_data_factory;
    options.thread_local_data_factory = &thread_local_data_factory;

    // Instance of your service.
    brpc::Server server;
    EchoServiceWithLocalData echo_service_impl;
    if (server.AddService(&echo_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) !=
        0) {
        LOG(ERROR) << "Failed to add service";
        return -1;
    }

    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Failed to start EchoServer";
        return -1;
    }

    server.RunUntilAskedToQuit();
    CHECK_EQ(ntls, 0);
    CHECK_EQ(nsd, 0);
    return 0;
}
