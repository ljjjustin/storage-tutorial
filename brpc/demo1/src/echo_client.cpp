#include <gflags/gflags.h>
#include "brpc/channel.h"
#include "butil/logging.h"
#include "echo.pb.h"


DEFINE_string(protocol, "h2:json", "protocol type. default is xxx");
DEFINE_string(conn_type, "pooled", "Connection type. Available values: single, pooled, short");
DEFINE_string(server_addr, "0.0.0.0:18000", "IP address of server");
DEFINE_int32(interval_ms, 1000, "Milliseconds between consecutive requests.");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds.");


int main() {
    std::cout << "brpc echo client starting..." << std::endl;

    brpc::ChannelOptions options;
    //options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_conn_type;
    options.timeout_ms = FLAGS_timeout_ms;

    brpc::Channel channel;
    if (channel.Init(FLAGS_server_addr.c_str(), &options) != 0) {
        LOG(ERROR) << "Failed to initialize channel.";
        return -1;
    }

    demo1::EchoService_Stub stub(&channel);

    int log_id = 0;
    while (!brpc::IsAskedToQuit()) {
        demo1::EchoRequest request;
        request.set_message("hello brpc server");

        brpc::Controller ctrl;
        ctrl.set_log_id(log_id++);

        demo1::EchoResponse response;
        stub.Echo(&ctrl, &request, &response, NULL);
        if (!ctrl.Failed()) {
            LOG(INFO) << "Received response from " << ctrl.remote_side()
                << " to " << ctrl.local_side()
                << ": " << response.message()
                << " latency=" << ctrl.latency_us() << "us";
        } else {
            LOG(WARNING) << ctrl.ErrorText();
        }
        usleep(FLAGS_interval_ms * 1000L);
    }

    LOG(INFO) << "brpc client is going to quit";

    return 0;
}
