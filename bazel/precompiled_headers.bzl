load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load('@rules_cc//cc:action_names.bzl', 'CPP_COMPILE_ACTION_NAME')

def _global_flags(ctx, cc_toolchain):
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
    )
    compile_variables = cc_common.create_compile_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        user_compile_flags = ctx.fragments.cpp.cxxopts + ctx.fragments.cpp.copts,
    )
    tc_flags = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = CPP_COMPILE_ACTION_NAME,
        variables = compile_variables,
    )

    if cc_toolchain.needs_pic_for_dynamic_libraries(feature_configuration=feature_configuration):
      return tc_flags + ['-fPIC']

    return tc_flags

def _precompiled_headers_impl(ctx):
  main_file = ctx.attr.main.files.to_list()[0]

  args = ctx.actions.args()

  # add args specified by the toolchain and on the command line
  cc_toolchain = find_cpp_toolchain(ctx)
  args.add_all(_global_flags(ctx, cc_toolchain))

  # collect headers, include paths and defines of dependencies
  headers = []
  for dep in ctx.attr.deps:
    if not CcInfo in dep:
      fail('dep arguments must provide CcInfo (e.g: cc_library)')

    # collect exported header files
    compilation_context = dep[CcInfo].compilation_context
    headers.append(compilation_context.headers)

    # add defines
    for define in compilation_context.defines.to_list():
      args.add('-D' + define)

    # add include dirs
    for i in compilation_context.includes.to_list():
      args.add('-I' + i)

    args.add_all(compilation_context.quote_includes.to_list(), before_each = '-iquote')

    args.add_all(compilation_context.system_includes.to_list(), before_each = '-isystem')

  inputs = depset(direct=[main_file], transitive=headers+[cc_toolchain.all_files])

  # add args specified for this rule
  args.add_all(ctx.attr.copts)

  # force compilation of header
  args.add('-xc++-header')
  args.add(main_file.path)

  # specify output
  output = ctx.actions.declare_file(main_file.basename + '.gch')
  args.add('-o', output.path)

  # Unless -MD is specified while creating the precompiled file,
  # the .d file of the user of the precompiled file will not
  # show the precompiled file: therefore bazel will not rebuild
  # the user if the pch file changes.
  args.add('-MD')
  args.add('-MF', "/dev/null")

  ctx.actions.run(
    inputs = inputs,
    outputs = [output],
    executable = cc_toolchain.compiler_executable,
    arguments = [args],
    mnemonic = 'PrecompileHdrs',
    progress_message = 'Pre-compiling header: ' + main_file.basename,
  )

  # create a CcInfo output that cc_binary rules can depend on
  compilation_context = cc_common.create_compilation_context(
    headers = depset(direct=[output, main_file]),
    includes = depset(direct=[output.dirname]),
  )
  main_cc_info = CcInfo(compilation_context = compilation_context, linking_context = None)
  cc_info = cc_common.merge_cc_infos(
      direct_cc_infos = [main_cc_info],
      cc_infos = [dep[CcInfo] for dep in ctx.attr.deps if CcInfo in dep],
      # before bazel 3.2:
      #cc_infos = [main_cc_info] + [dep[CcInfo] for dep in ctx.attr.deps if CcInfo in dep]
  )

  return [ cc_info ]

precompiled_headers = rule(
  implementation = _precompiled_headers_impl,
  attrs = {
    "main": attr.label(allow_files=True, mandatory=True),
    "deps": attr.label_list(),
    "copts": attr.string_list(),

    "_cc_toolchain": attr.label(default = Label("@bazel_tools//tools/cpp:current_cc_toolchain")),
  },
  toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
  fragments = ["cpp"],
)
