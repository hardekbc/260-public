package(default_visibility = ["//visibility:public"])

cc_library(
    name = "standard_includes",
    hdrs = ["standard_includes.h"],
)

cc_library(
    name = "tokenizer",
    hdrs = ["tokenizer.h"],
    deps = [":standard_includes"],
)

cc_test(
    name = "tokenizer_test",
    srcs = ["tokenizer_test.cc"],
    deps = [":tokenizer"],
)
