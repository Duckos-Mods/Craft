
project('Craft', 'cpp', default_options: ['cpp_std=c++latest'])

cmake = import('cmake')
zasm_opts = cmake.subproject_options()
zasm_opts.add_cmake_defines({
    'ZASM_BUILD_EXAMPLES': false,
    'ZASM_BUILD_BENCHMARKS': false,
    'ZASM_BUILD_TESTS': false
})

zasm_sub = cmake.subproject('zasm')

zasm_dep = zasm_sub.dependency('zasm')

sources = [
    'src/Craft.hpp',
    'src/Internal/CraftMemory.cpp',
    'src/Internal/CraftUtils.hpp',
    'src/Internal/OS.hpp',
    'src/Internal/CraftMemory.hpp',
    'src/Internal/CraftHook.cpp',
    'src/Internal/CraftHook.hpp',
    'src/Internal/CraftASM.cpp',
    'src/Internal/CraftASM.hpp',
    'src/Internal/CraftRegisterCalculations.hpp',
    'src/Internal/CraftRegisterCalculations.cpp',
    'src/Internal/CraftThreads.hpp',
    'src/Internal/CraftThreads.cpp',
    

]    
headers = [
    'src/Craft.hpp',
    'src/Internal/CraftUtils.hpp',
    'src/Internal/OS.hpp',
    'src/Internal/CraftASM.hpp',
    'src/Internal/CraftHook.hpp',
    'src/Internal/CraftRegisterCalculations.hpp',
    'src/Internal/CraftThreads.hpp',
    'src/Internal/CraftMemory.hpp'
]

source_include = include_directories('src/')

testing = get_option('enableTests') 
testing = true
if testing 
    message('Tests enabled')
else
    message('Tests disabled')
endif
craft_lib = static_library('Craft', sources, include_directories : source_include, dependencies: zasm_dep)

install_headers(headers, subdir: 'Craft')

if testing
    testing_sources = [
        'test/main.cpp'
    ]
    executable(
        'CraftTester',
        testing_sources,
        include_directories: source_include,
        link_with: craft_lib, 
        dependencies: zasm_dep
    )
    
endif