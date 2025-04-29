add_repositories("xsky-repo git@github.xsky.com:ceph/xmake-repo.git")

add_requires("rocksdb")

target("demo1")
    set_kind("binary")
    add_files("src/demo1.cpp")
    add_packages("rocksdb", {public=true})
