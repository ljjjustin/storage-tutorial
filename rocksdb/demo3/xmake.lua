add_repositories("xsky-repo git@github.xsky.com:ceph/xmake-repo.git")

add_rules("plugin.compile_commands.autoupdate", {outputdir = ".", lsp = "clangd"})

add_requires("rocksdb")

target("demo1")
    set_kind("binary")
    add_files("src/demo3.cpp")
    add_packages("rocksdb", {public=true})
