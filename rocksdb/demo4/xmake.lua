add_repositories("xsky-repo git@github.xsky.com:ceph/xmake-repo.git")

add_requires("gflags")
add_requires("rocksdb")

target("demo4")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("rocksdb", {public=true})
    add_packages("gflags", {public=true})
