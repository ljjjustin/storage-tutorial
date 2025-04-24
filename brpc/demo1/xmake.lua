add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".", lsp = "clangd"})

add_requires("brpc 1.10.0", {debug = is_mode("debug"), configs = {enable_glog = true}})
add_requires("protobuf-cpp 3.19.4")


target("proto_gen")
    set_kind("object")
    add_rules("protobuf.cpp")
    add_packages("protobuf-cpp", {public = true})
    add_files("src/*.proto", {proto_public = true})


target("echo_server")
    set_kind("binary")
    add_files("src/echo_server.cpp")
    add_deps("proto_gen")
    add_packages("brpc", {public = true})


target("echo_client")
    set_kind("binary")
    add_files("src/echo_client.cpp")
    add_deps("proto_gen")
    add_packages("brpc", {public = true})
