#include <gflags/gflags.h>
#include "brpc/server.h"
#include "butil/logging.h"
#include "json2pb/pb_to_json.h"
#include "echo.pb.h"

DEFINE_int32(port, 18000, "TCP port of this server");
DEFINE_string(listen_addr, "", "Server listen address");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
        "read/write operations during the last `idle_timeout_s`");

namespace demo1 {
class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() {}
    virtual ~EchoServiceImpl() {}

    virtual void Echo(google::protobuf::RpcController *ctrl_base,
            const EchoRequest * request,
            EchoResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);

        brpc::Controller *ctrl = static_cast<brpc::Controller*>(ctrl_base);
        ctrl->set_after_rpc_resp_fn(std::bind(&EchoServiceImpl::CallAfterRpc,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        LOG(INFO) << "Received request[log_id=" << ctrl->log_id()
                  << "] from" << ctrl->remote_side() << " to " << ctrl->local_side()
                  << ": " << request->message();

        response->set_message(request->message());
    }

    static void CallAfterRpc(brpc::Controller *ctrl,
            const google::protobuf::Message *req,
            const google::protobuf::Message *res) {
        std::string req_str;
        std::string res_str;
        json2pb::ProtoMessageToJson(*req, &req_str, NULL);
        json2pb::ProtoMessageToJson(*res, &res_str, NULL);
        LOG(INFO) << "req: " << req_str << " res: " << res_str;
    }
};
} // namespace demo1

int main(int argc, char *argv[]) {
    std::cout << "brpc echo server" << std::endl;
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    brpc::Server server;
    demo1::EchoServiceImpl echo_service_impl;

    if (server.AddService(&echo_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Failed to add service";
        return -1;
    }

    butil::EndPoint endpoint;
    if (!FLAGS_listen_addr.empty()) {
        if (butil::str2endpoint(FLAGS_listen_addr.c_str(), &endpoint) < 0 ) {
            LOG(ERROR) << "Invalid listen address: " << FLAGS_listen_addr;
            return -1;
        }
    } else {
        endpoint = butil::EndPoint(butil::IP_ANY, FLAGS_port);
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(endpoint, &options) != 0 ) {
        LOG(ERROR) << "Failed to start EchoServer";
        return -1;
    }

    server.RunUntilAskedToQuit();

    return 0;
}
