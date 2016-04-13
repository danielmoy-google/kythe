workspace(name = "io_kythe")

load("//:version.bzl", "check_version")

check_version("0.2.1")

load("//tools/cpp:clang_configure.bzl", "clang_configure")

clang_configure()

load("//tools/go:go_configure.bzl", "go_configure")

go_configure()

load("//tools:node_configure.bzl", "node_configure")

node_configure()

load("//tools/build_rules/config:system.bzl", "cc_system_package")

cc_system_package(
    name = "libcrypto",
    default = "/usr/local/opt/openssl",
    envvar = "OPENSSL_HOME",
)

cc_system_package(
    name = "libuuid",
    default = "/usr/local/opt/ossp-uuid",
    envvar = "UUID_HOME",
    modname = "uuid",
)

cc_system_package(
    name = "libmemcached",
    default = "/usr/local/opt/libmemcached",
    envvar = "MEMCACHED_HOME",
)

new_git_repository(
    name = "com_github_google_googletest",
    build_file = "third_party/googletest.BUILD",
    remote = "https://github.com/google/googletest.git",
    tag = "release-1.7.0",
)

bind(
    name = "googletest",
    actual = "@com_github_google_googletest//:googletest",
)

bind(
    name = "googletest/license",
    actual = "@com_github_google_googletest//:license",
)

new_git_repository(
    name = "com_github_gflags_gflags",
    build_file = "third_party/googleflags.BUILD",
    commit = "58345b18d92892a170d61a76c5dd2d290413bdd7",
    remote = "https://github.com/gflags/gflags.git",
)

bind(
    name = "gflags",
    actual = "@com_github_gflags_gflags//:gflags",
)

bind(
    name = "gflags/license",
    actual = "@com_github_gflags_gflags//:license",
)

git_repository(
    name = "com_googlesource_code_re2",
    commit = "63e4dbd1996d2cf723f740e08521a67ad66a09de",
    remote = "https://code.googlesource.com/re2",
)

bind(
    name = "re2",
    actual = "@com_googlesource_code_re2//:re2",
)

bind(
    name = "re2/license",
    actual = "@com_googlesource_code_re2//:LICENSE",
)

new_git_repository(
    name = "com_github_google_glog",
    build_file = "third_party/googlelog/BUILD.remote",
    commit = "1b0b08c8dda1659027677966b03a3ff3c488e549",
    remote = "https://github.com/google/glog.git",
)

bind(
    name = "glog",
    actual = "@com_github_google_glog//:glog",
)

bind(
    name = "glog/license",
    actual = "@com_github_google_glog//:license",
)

maven_jar(
    name = "maven_guava",
    artifact = "com.google.guava:guava:19.0",
    sha1 = "6ce200f6b23222af3d8abb6b6459e6c44f4bb0e9",
)

http_file(
    name = "com_apache_org_license_2_0",
    sha256 = "cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30",
    url = "http://www.apache.org/licenses/LICENSE-2.0.txt",
)

bind(
    name = "guava",
    actual = "@maven_guava//jar",
)

bind(
    name = "guava/license",
    actual = "@com_apache_org_license_2_0//file",
)

bind(
    name = "libcurl",
    actual = "//third_party:libcurl",
)

bind(
    name = "junit4",
    actual = "//third_party:junit4",
)

bind(
    name = "zlib",
    actual = "//third_party/zlib",
)

bind(
    name = "re2j",
    actual = "//third_party/re2j",
)

bind(
    name = "proto/protobuf",
    actual = "//third_party/proto:protobuf",
)

bind(
    name = "proto/protobuf_java",
    actual = "//third_party/proto:protobuf_java",
)

bind(
    name = "proto/any_proto",
    actual = "//third_party/proto:any_proto",
)

bind(
    name = "proto/any_proto_cc",
    actual = "//third_party/proto:any_proto_cc",
)

bind(
    name = "proto/any_proto_java",
    actual = "//third_party/proto:any_proto_java",
)

bind(
    name = "proto/any_proto_go",
    actual = "//third_party/proto:any_proto_go",
)

bind(
    name = "grpc-java",
    actual = "//third_party/grpc-java",
)

bind(
    name = "rapidjson",
    actual = "//third_party/rapidjson",
)

bind(
    name = "gson",
    actual = "//third_party/gson",
)

bind(
    name = "gson/proto",
    actual = "//third_party/gson:proto",
)

bind(
    name = "jcommander",
    actual = "//third_party/jcommander",
)

bind(
    name = "jq",
    actual = "//third_party/jq",
)

bind(
    name = "truth",
    actual = "//third_party/truth",
)

bind(
    name = "go_package_prefix",
    actual = "//:go_package_prefix",
)

new_git_repository(
    name = "go_gogo_protobuf",
    build_file = "third_party/go/gogo_protobuf.BUILD",
    commit = "4f262e4b0f3a6cea646e15798109335551e21756",
    remote = "https://github.com/gogo/protobuf.git",
)

bind(
    name = "android/sdk",
    actual = "//:nothing",
)
