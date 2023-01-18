package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ir",
    hdrs = [
        "ir.h",
        "irvisitor.h",
        "ir_tostring_visitor.h",
        "debug_visitor.h",
    ],
    srcs = ["ir.cc"],
    deps = [
        "//util:standard_includes",
        "//util:tokenizer",
    ],
)

cc_library(
    name = "irbuilder",
    hdrs = ["irbuilder.h"],
    srcs = ["irbuilder.cc"],
    deps = [
        ":ir",
        "//util:standard_includes",
    ],
)

cc_test(
    name = "ir_test",
    srcs = ["ir_test.cc"],
    deps = [":ir"],
    data = glob(["testdata/**"]),
)

cc_test(
    name = "irbuilder_test",
    srcs = ["irbuilder_test.cc"],
    deps = [":irbuilder"],
)
