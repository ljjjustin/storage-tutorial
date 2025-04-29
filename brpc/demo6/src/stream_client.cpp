#include <brpc/channel.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

#include "demo6.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with the request");
DEFINE_int32(timeout_ms, 2000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_string(server, "0.0.0.0:18000", "IP address of server");

class StreamClientReceiver : public brpc::StreamInputHandler {
public:
    virtual int on_received_messages(brpc::StreamId id,
                                     butil::IOBuf *const messages[], size_t size) {
        std::ostringstream os;
        for (size_t i = 0; i < size; i++) {
            os << "msg[" << i << "]=" << *messages[i];
        }
        LOG(INFO) << "Received from Stream=" << id << ": " << os.str();
        return 0;
    }
    virtual void on_idle_timeout(brpc::StreamId id) {
        LOG(INFO) << "Stream=" << id << " has no data transmission for a while";
    }
    virtual void on_closed(brpc::StreamId id) {
        LOG(INFO) << "Stream=" << id << " is closed";
    }
    virtual void on_finished(brpc::StreamId id, int32_t finish_code) {
        LOG(INFO) << "Stream=" << id << " is finished, code=" << finish_code;
    }
};

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

    brpc::Controller ctrl[3];
    StreamClientReceiver receiver;
    brpc::StreamOptions stream_options;
    stream_options.handler = &receiver;
    brpc::StreamId streams[3];
    for (auto i = 0; i < 3; i++) {
        if (brpc::StreamCreate(&streams[i], ctrl[i], &stream_options) != 0) {
            LOG(ERROR) << "Failed to create stream";
            return -1;
        }
        LOG(INFO) << "Created stream=" << streams[i];
    }

    demo6::EchoRequest request;
    request.set_message("I'm a RPC to connect stream");

    demo6::EchoService_Stub stub(&channel);
    demo6::EchoResponse response;
    for (auto i = 0; i < 3; i++) {
        stub.Echo(&ctrl[i], &request, &response, NULL);
        if (ctrl[i].Failed()) {
            LOG(ERROR) << "Failed to connect stream" << ctrl[i].ErrorText();
            return -1;
        }
    }

    while (!brpc::IsAskedToQuit()) {
        butil::IOBuf msg1;
        msg1.append("abcdefghijklmnopqrstuvwxyz");
        CHECK_EQ(0, brpc::StreamWrite(streams[0], msg1));

        butil::IOBuf msg2;
        msg2.append("01234567890");
        CHECK_EQ(0, brpc::StreamWrite(streams[1], msg2));
        sleep(1);

        butil::IOBuf msg3;
        msg3.append("hello world");
        CHECK_EQ(0, brpc::StreamWrite(streams[2], msg3));
        sleep(1);
    }

    for (auto stream : streams) {
        CHECK_EQ(0, brpc::StreamClose(stream));
    }
    LOG(INFO) << "StreamEchoClient is going to quit";

    return 0;
}
