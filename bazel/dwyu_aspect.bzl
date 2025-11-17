load("@depend_on_what_you_use//:defs.bzl", "dwyu_aspect_factory")

dwyu_aspect = dwyu_aspect_factory(
    recursive = False,
    experimental_no_preprocessor = False,
    ignore_cc_toolchain_headers = True,
    use_implementation_deps = True,
    skip_external_targets = False,
)
