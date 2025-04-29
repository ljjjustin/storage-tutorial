#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>

#include "demo6.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with the request");
DEFINE_int32(port, 18000, "tcp port of this server");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no"
             "read/write operations during the last `idle_timeout_s`");

class StreamReceiver : public brpc::StreamInputHandler {
public:
    virtual int on_received_messages(brpc::StreamId id,
                                     butil::IOBuf *const messages[], size_t size) {
        std::ostringstream os;
        for (size_t i = 0; i < size; i++) {
            os << "msg[" << i << "]=" << *messages[i];
        }
        auto res = brpc::StreamWrite(id, *messages[0]);
        LOG(INFO) << "Received from Stream=" << id << ": " << os.str()
                  << " and write back result: " << res;
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

class StreamEchoService : public demo6::EchoService {
public:
    StreamEchoService() {}
    virtual ~StreamEchoService() {
        closeStreams();
    }

    void Echo(google::protobuf::RpcController *ctrl_base, const demo6::EchoRequest *,
              demo6::EchoResponse *response, google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);

        brpc::Controller *ctrl = static_cast<brpc::Controller *>(ctrl_base);
        brpc::StreamOptions stream_options;
        stream_options.handler = &_receiver;
        brpc::StreamId _sid;
        if (brpc::StreamAccept(&_sid, *ctrl, &stream_options) != 0) {
            ctrl->SetFailed("Failed to accept stream");
            return;
        }
        _sids.push_back(_sid);
        response->set_message("Accepted stream");
    }

private:
    void closeStreams() {
        for (auto i = 0; i < _sids.size(); ++i) {
            brpc::StreamClose(_sids[i]);
        }
        _sids.clear();
    }
    StreamReceiver _receiver;
    std::vector<brpc::StreamId> _sids;
};

int main(int argc, char *argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    StreamEchoService echo_service;
    brpc::Server server;

    if (server.AddService(&echo_service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Failed to add http_service";
        return -1;
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Failed to start EchoServer";
        return -1;
    }

    // Wait until ctrl+c is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();

    return 0;
}
