package(default_visibility = ["//visibility:public"])

cc_library(
    name = "trivial_example",
    hdrs = ["trivial_example.h"],
    srcs = ["trivial_example.cc"],
    deps = [
        "//ir:ir",
        "//util:standard_includes",
    ],
)

cc_test(
    name = "trivial_example_test",
    srcs = ["trivial_example_test.cc"],
    deps = [":trivial_example"],
)
