#include <brpc/channel.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

DEFINE_string(d, "", "POST this data to the http server");
DEFINE_int32(timeout_ms, 2000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_string(protocol, "http", "Client-side protocol");

namespace brpc {
DECLARE_bool(http_verbose);
}

int main(int argc, char *argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 2) {
        LOG(ERROR) << "Usage: ./http_client \"http(s)://server\"";
        return -1;
    }
    char *url = argv[1];

    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.max_retry = FLAGS_max_retry;
    options.timeout_ms = FLAGS_timeout_ms;

    if (channel.Init(url, &options) != 0) {
        LOG(ERROR) << "Failed to initialize channel";
        return -1;
    }

    brpc::Controller ctrl;
    ctrl.http_request().uri() = url;
    if (!FLAGS_d.empty()) {
        ctrl.http_request().set_method(brpc::HTTP_METHOD_POST);
        ctrl.request_attachment().append(FLAGS_d);
    }

    channel.CallMethod(NULL, &ctrl, NULL, NULL, NULL);
    if (ctrl.Failed()) {
        std::cerr << ctrl.ErrorText() << std::endl;
        return -1;
    }

    if (!brpc::FLAGS_http_verbose) {
        std::cout << ctrl.response_attachment() << std::endl;
    }

    return 0;
}
