add_repositories("xsky-repo git@github.xsky.com:ceph/xmake-repo.git")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".", lsp = "clangd"})

add_requires("protoc", "protobuf-cpp")
add_requireconfs("protoc.protobuf-cpp", {version="3.15.8"})
add_requires("brpc 1.10.0", {debug = is_mode("debug"), configs = {enable_glog = true}})


target("proto_gen")
    set_kind("object")
    add_rules("protobuf.cpp")
    add_packages("protobuf-cpp", {public = true})
    add_files("src/*.proto", {proto_public = true})


target("stream_server")
    set_kind("binary")
    add_files("src/stream_server.cpp")
    add_deps("proto_gen")
    add_packages("brpc", {public = true})


target("stream_client")
    set_kind("binary")
    add_files("src/stream_client.cpp")
    add_deps("proto_gen")
    add_packages("brpc", {public = true})
