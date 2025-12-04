load("@depend_on_what_you_use//:defs.bzl", "dwyu_aspect_factory")

dwyu_aspect = dwyu_aspect_factory(experimental_no_preprocessor = True, skip_external_targets = True, use_implementation_deps = True, ignore_cc_toolchain_headers = True, ignored_includes = Label("@//:dwyu_ignore_includes.json"))
