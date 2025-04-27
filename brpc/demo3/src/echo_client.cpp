#include <butil/logging.h>
#include <bvar/bvar.h>
#include <gflags/gflags.h>

#include "brpc/channel.h"
#include "echo.pb.h"

DEFINE_int32(thread_num, 2000, "Number of threads to send requests");
DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_int32(request_size, 16, "Bytes of each request");
DEFINE_int32(attachment_size, 0,
             "Carry so many bytes attachment along with requests");
DEFINE_string(protocol, "baidu_std", "Protocol type.");
DEFINE_string(connection_type, "",
              "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:18000", "IP address of server");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");

std::string g_request;
std::string g_attachment;

bvar::LatencyRecorder g_latency_recorder("client");

static void *sender(void *arg) {
    demo3::EchoService_Stub stub(static_cast<google::protobuf::RpcChannel *>(arg));

    int log_id = 0;
    while (!brpc::IsAskedToQuit()) {
        // 同步接收响应，可以安全的使用栈上的变量
        demo3::EchoRequest request;
        demo3::EchoResponse response;
        brpc::Controller ctrl;

        request.set_message(g_request);
        ctrl.set_log_id(log_id++);
        if (!g_attachment.empty()) {
            ctrl.request_attachment().append(g_attachment);
        }

        const int64_t start_time = butil::cpuwide_time_us();

        // waits until the response comes back or error occurs
        // including timeout.
        stub.Echo(&ctrl, &request, &response, NULL);

        const int64_t end_time = butil::cpuwide_time_us();
        const int64_t elp = end_time - start_time;
        if (!ctrl.Failed()) {
            g_latency_recorder << elp;
        } else {
            CHECK(brpc::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << ctrl.ErrorText() << " latency=" << elp;
            bthread_usleep(50000);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms;
    options.max_retry = FLAGS_max_retry;

    if (channel.Init(FLAGS_server.c_str(), "", &options) != 0) {
        LOG(ERROR) << "Failed to initialize channel";
        return -1;
    }

    if (FLAGS_attachment_size > 0) {
        g_attachment.resize(FLAGS_attachment_size, 'A');
    }

    if (FLAGS_request_size <= 0) {
        LOG(ERROR) << "Bad request_size" << FLAGS_request_size;
        return -1;
    }
    g_request.resize(FLAGS_request_size, 'r');

    std::vector<bthread_t> bids;
    std::vector<pthread_t> pids;
    if (!FLAGS_use_bthread) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(&pids[i], NULL, sender, &channel) != 0) {
                LOG(ERROR) << "Failed to create pthread";
                return -1;
            }
        }
    } else {
        bids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (bthread_start_background(&bids[i], NULL, sender, &channel) != 0) {
                LOG(ERROR) << "Failed to create bthread";
                return -1;
            }
        }
    }

    while (!brpc::IsAskedToQuit()) {
        sleep(1);
        LOG(INFO) << "Sending EchoRequest at qps=" << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    return 0;
}
