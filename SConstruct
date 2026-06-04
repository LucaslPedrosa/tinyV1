#!/usr/bin/env python

import os

env = SConscript("external/godot-cpp/SConstruct")

env.Append(CPPPATH=["src"])

sources = Glob("src/*.cpp")
target_path = os.path.join("bin", "libtinyv1" + env["suffix"] + env["SHLIBSUFFIX"])

library = env.SharedLibrary(target_path, source=sources)

Default(library)
