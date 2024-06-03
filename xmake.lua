add_rules("mode.debug", "mode.release")
add_requires("llvm-prebuilt 16.0.6")

set_languages("c++23")
target("Craft")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    add_headerfiles("src/**.hpp")
    add_packages("llvm-prebuilt", {public = true})
    set_runtimes("MD")


target("CraftTests")
    set_kind("binary")
    add_files("test/**.cpp")
    add_headerfiles("test/**.hpp")
    set_runtimes("MD")
    add_deps("Craft")