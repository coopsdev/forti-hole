project('forti-hole', 'cpp',
        version : '0.1.3',
        default_options : ['warning_level=3', 'cpp_std=c++23'])

source_root = meson.source_root()

forti_api_dep = dependency('forti_api', required: true)
yaml_dep = dependency('yaml-cpp', required: true)
libcurl_dep = dependency('libcurl', required: true)

global_deps = [forti_api_dep, yaml_dep, libcurl_dep]

sources = []
foreach cpp_file : run_command('find', source_root + '/src', '-type', 'f', '-name', '*.cpp', check: true).stdout().strip().split('\n')
        sources += files(cpp_file)
endforeach

main_cpp = files(source_root + '/main.cpp')

forti_hole = executable('forti-hole', sources + main_cpp, dependencies: global_deps, install : true)

if get_option('buildtype') == 'debug'
        gtest_dep = dependency('gtest', required : true, main : true)

        test_deps = global_deps + gtest_dep

        test_sources = sources
        foreach cpp_file : run_command('find', source_root + '/tests', '-type', 'f', '-name', '*.cpp', check: true).stdout().strip().split('\n')
                test_sources += files(cpp_file)
        endforeach

        test('runTests', executable('runTests', test_sources, dependencies: test_deps))
endif