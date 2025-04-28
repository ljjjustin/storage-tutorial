#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>

#include "demo4.pb.h"

DEFINE_int32(port, 18000, "tcp port of this server");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no"
             "read/write operations during the last `idle_timeout_s`");

namespace demo4 {
class HttpServiceImpl : public HttpService {
public:
    HttpServiceImpl() {}
    virtual ~HttpServiceImpl() {}

    void Echo(google::protobuf::RpcController *ctrl_base, const HttpRequest *,
              HttpResponse *, google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        brpc::Controller *ctrl = static_cast<brpc::Controller *>(ctrl_base);

        // optional: set a callback function which is called after response is sent
        // and before ctrl/req/res is destructed.
        ctrl->set_after_rpc_resp_fn(
            std::bind(&HttpServiceImpl::CallAfterRpc, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));

        ctrl->http_response().set_content_type("text/plain");

        auto uri = ctrl->http_request().uri();
        butil::IOBufBuilder os;
        os << "queries: ";
        for (brpc::URI::QueryIterator it = uri.QueryBegin(); it != uri.QueryEnd();
             ++it) {
            os << ' ' << it->first << '=' << it->second;
        }
        os << "\nbody: " << ctrl->request_attachment() << '\n';
        os.move_to(ctrl->response_attachment());
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
}  // namespace demo4

int main(int argc, char *argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    demo4::HttpServiceImpl http_service;
    brpc::Server server;

    if (server.AddService(&http_service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Failed to add http_service";
        return -1;
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Failed to start HttpServer";
        return -1;
    }

    server.RunUntilAskedToQuit();

    return 0;
}
