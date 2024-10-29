# -----------------------------
# Options effecting formatting.
# -----------------------------
with section("parse"):
    additional_commands = {
        "declare_module": {
            "pargs": "*",
            "flags": ["ALWAYS_BUILD", "EXCLUDE_FROM_ALL"],
            "kwargs": {
                "NAME": 1,
                "DEPENDS_ON_MODULES": '*',
                "DEPENDS_ON_MODULES_FOR_TESTING": "*",
                "DEPENDS_ON_EXTERNAL_PROJECTS": '*',
            }
        },
        "define_module_library": {
            "pargs": "*",
            "flags": ["NOINSTALL"],
            "kwargs": {
                "NAME": 1,
                "SOURCES": '*',
                "PUBLIC_INCLUDE_PATHS": '*',
                "PRIVATE_INCLUDE_PATHS": '*',
                "SYSTEM_PUBLIC_INCLUDE_PATHS": '*',
                "SYSTEM_PRIVATE_INCLUDE_PATHS": '*',
                "PUBLIC_LINK_LIBS": '*',
                "PRIVATE_LINK_LIBS": '*',
            }
        },
        "define_module_example": {
            "pargs": 0,
            "flags": [],
            "kwargs": {
                "NAME": 1,
                "SOURCES": '*',
                "PUBLIC_INCLUDE_PATHS": '*',
                "PRIVATE_INCLUDE_PATHS": '*',
                "PUBLIC_LINK_LIBS": '*',
                "PRIVATE_LINK_LIBS": '*',
            }
        },
        "define_module_executable": {
            "pargs": 0,
            "flags": [],
            "kwargs": {
                "NAME": 1,
                "SOURCES": '*',
                "PUBLIC_INCLUDE_PATHS": '*',
                "PRIVATE_INCLUDE_PATHS": '*',
                "PUBLIC_LINK_LIBS": '*',
                "PRIVATE_LINK_LIBS": '*',
            }
        },
        "define_module_executable": {
            "pargs": 0,
            "flags": [],
            "kwargs": {
                "NAME": 1,
                "SOURCES": '*',
                "PUBLIC_LINK_LIBS": '*',
                "PRIVATE_LINK_LIBS": '*',
            }
        },
        "define_module_test": {
            "pargs": 0,
            "flags": [],
            "kwargs": {
                "NAME": 1,
                "COMMAND": 1,
                "WORKING_DIRECTORY": 1,
                "SOURCES": '*',
                "PUBLIC_INCLUDE_PATHS": '*',
                "PRIVATE_INCLUDE_PATHS": '*',
                "PUBLIC_LINK_LIBS": '*',
                "PRIVATE_LINK_LIBS": '*',
            }
        },
        "add_cmake_dependency": {
            "pargs": 0,
            "flags": [],
            "kwargs": {
                "NAME": 1,
                "VERSION": 1,
                "GIT_TAG": 1,
                "GIT_REPOSITORY": 1,
                "SOURCE_SUBDIR": 1,
                "CMAKE_ARGS": '*',
                "DEPENDS": '*',
            }
        },
    }

with section("format"):

  # How wide to allow formatted cmake files
  line_width = 120

  # How many spaces to tab for indent
  tab_size = 2

  # If true, separate flow control names from their parentheses with a space
  separate_ctrl_name_with_space = False

  # If true, separate function names from parentheses with a space
  separate_fn_name_with_space = False

  # If a statement is wrapped to more than one line, than dangle the closing
  # parenthesis on its own line.
  dangle_parens = True
