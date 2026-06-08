#!/usr/bin/env python

import os

env = SConscript("external/godot-cpp/SConstruct")

game_env = env.Clone()
game_env.Append(CXXFLAGS=["-std=gnu++23"])
game_env.Append(CPPPATH=["src"])

game_env.VariantDir("build/src", "src", duplicate=0)
sources = Glob("build/src/*.cpp")
sources += Glob("build/src/game_simulation/*.cpp")
sources += Glob("build/src/rendering/*.cpp")
target_path = os.path.join("bin", "libtinyv1" + env["suffix"] + env["SHLIBSUFFIX"])

library = game_env.SharedLibrary(target_path, source=sources)

Default(library)
