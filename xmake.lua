add_rules("mode.debug", "mode.release")
add_requires("llvm-prebuilt 16.0.6")
add_requires("zydis")

MSVC_COMPILE_OPTIONS = {
    ["ldflags"] = {
        "/DEBUG:FULL"
    },
    ["shflags"] = {
        "/DEBUG:FULL"
    }
}

local function apply_options_to_target(target)
    for option, values in pairs(MSVC_COMPILE_OPTIONS) do
        target:add(option, values, { tools = {"clang_cl", "cl", "link"}})
        print("Added option: " .. option .. " with values: " .. table.concat(values, ", "))
    end  

end


set_languages("c++23")
target("Craft")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    add_headerfiles("src/**.hpp")
    add_packages("llvm-prebuilt", {public = true})
    add_packages("zydis", {public = false})
    set_runtimes("MD")
    on_config(function(target)
        print("Configuring Craft")
        apply_options_to_target(target)
    end)
    set_symbols("debug")

target("CraftTests")
    set_kind("binary")
    add_files("test/**.cpp")
    add_headerfiles("test/**.hpp")
    set_runtimes("MD")
    add_deps("Craft")
    on_config(function(target)
        print("Configuring CraftTests")
        apply_options_to_target(target)
    end)
    set_symbols("debug")