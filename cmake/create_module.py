#!/usr/bin/env python3
#
# Copyright (C) 2018 GRAPE Contributors
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#

import os
import shutil
import sys
import fileinput


def create_module(module_name, prefix_name="heph"):
    """
    Generate a template module to later populate. The module is created in the current location
    :param module_name: Name of the module
    :param prefix_name: Name of the namespace prefix, default: "heph"
    :return:
    """

    # Copy the template over
    source_location = os.path.dirname(os.path.abspath(__file__))
    shutil.copytree(source_location + "/module_template", module_name)

    # Include folder
    is_forkify_module = "forkify" in prefix_name
    include_folder = "hephaestus"
    if is_forkify_module:
        include_folder = "forkify"

    # Contributors header
    hephaestus_header = "2023-2024 HEPHAESTUS Contributors"
    forkify_header = "2024 FILICS Contributors"

    # rename newly created template files
    os.rename(
        os.path.join(module_name, "include", "hephaestus", "@module@", "@module@.h"),
        os.path.join(
            module_name, "include", "hephaestus", "@module@", f"{module_name}.h"
        ),
    )
    os.rename(
        os.path.join(module_name, "include", "hephaestus", "@module@"),
        os.path.join(module_name, "include", "hephaestus", module_name),
    )
    os.rename(
        os.path.join(module_name, "include", "hephaestus"),
        os.path.join(module_name, "include", include_folder),
    )
    os.rename(
        os.path.join(module_name, "src", "@module@.cpp"),
        os.path.join(module_name, "src", f"{module_name}.cpp"),
    )
    os.rename(
        os.path.join(module_name, "apps", "@module@_app.cpp"),
        os.path.join(module_name, "apps", f"{module_name}_app.cpp"),
    )

    # Replace all instances of @module@ in files with module name
    for subdir, _, files in os.walk(module_name):
        for f in files:
            file_path = os.path.join(subdir, f)
            with fileinput.FileInput(file_path, inplace=True) as file:
                for line in file:
                    new_line = line.replace("@module@", module_name)
                    new_line = new_line.replace("@prefix@", prefix_name)
                    new_line = new_line.replace("@include@", include_folder)
                    if is_forkify_module:
                        new_line = new_line.replace(hephaestus_header, forkify_header)
                    print(new_line, end="")
    return


if __name__ == "__main__":
    n_args = len(sys.argv)
    if n_args == 1 or n_args > 3:
        print("Usage: python3 create_module.py <module_name> <optional: prefix_name>")
        sys.exit(1)
    elif n_args == 2:
        create_module(sys.argv[1])
    elif n_args == 3:
        create_module(sys.argv[1], sys.argv[2])
