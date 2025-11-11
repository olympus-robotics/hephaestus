load("@depend_on_what_you_use//:defs.bzl", "dwyu_aspect_factory")

dwyu_aspect = dwyu_aspect_factory(
  experimental_no_preprocessor=True,
	ignore_cc_toolchain_headers=True,
	use_implementation_deps = True,
	skip_external_targets = True
)
