#include <brpc/channel.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include "demo5.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with the request");
DEFINE_int32(timeout_ms, 2000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_string(server, "0.0.0.0:18000", "IP address of server");

int main(int argc, char *argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.max_retry = FLAGS_max_retry;
    options.timeout_ms = FLAGS_timeout_ms;
    // protocol MUST be baidu_std
    options.protocol = brpc::PROTOCOL_BAIDU_STD;

    if (channel.Init(FLAGS_server.c_str(), &options) != 0) {
        LOG(ERROR) << "Failed to initialize channel";
        return -1;
    }

    brpc::Controller ctrl;
    brpc::StreamId stream;
    if (brpc::StreamCreate(&stream, ctrl, NULL) != 0) {
        LOG(ERROR) << "Failed to create stream";
        return -1;
    }
    LOG(INFO) << "Created stream=" << stream;

    demo5::EchoRequest request;
    request.set_message("I'm a RPC to connect stream");

    demo5::EchoService_Stub stub(&channel);
    demo5::EchoResponse response;
    stub.Echo(&ctrl, &request, &response, NULL);
    if (ctrl.Failed()) {
        LOG(ERROR) << "Failed to connect stream" << ctrl.ErrorText();
        return -1;
    }

    while (!brpc::IsAskedToQuit()) {
        butil::IOBuf msg1;
        msg1.append("abcdefghijklmnopqrstuvwxyz");
        CHECK_EQ(0, brpc::StreamWrite(stream, msg1));

        butil::IOBuf msg2;
        msg2.append("01234567890");
        CHECK_EQ(0, brpc::StreamWrite(stream, msg2));
        sleep(1);
    }

    CHECK_EQ(0, brpc::StreamClose(stream));
    LOG(INFO) << "StreamEchoClient is going to quit";

    return 0;
}
