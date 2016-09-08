{
    'variables': {
        'include_files': [
            'include/image/image.hpp',
            'include/image/encoder.hpp',
            'include/image/quantizer.hpp',
            'include/image/rescaler.hpp',
        ],
        'source_files': [
            'src/image.cpp',
            'src/encoder.cpp',
            'src/quantizer.cpp',
            'src/rescaler.cpp',
        ],
        'include_dirs': ['include',
            '<!(node -e require(\'v8pp\'))',
            '<!@(pkgdeps include nitrogen)'
        ],
    },
    'targets': [
        {
            'target_name': 'image',
            'cflags_cc+': ['-std=c++11', '-fexceptions'],
             'configurations': {
                'Debug': { 'msvs_settings': { 'VCCLCompilerTool': {
                    'ExceptionHandling': 1,
                    'RuntimeLibrary': 3, # MultiThreadedDebugDLL
                }}},
                'Release': { 'msvs_settings': { 'VCCLCompilerTool': {
                    'ExceptionHandling': 1,
                    'RuntimeLibrary': 2, # MultiThreadedDLL
                }}},
            },
           'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES' },
            'dependencies': [
                'extern/libpng/libpng.gyp:libpng',
                'extern/mozjpeg/mozjpeg.gyp:mozjpeg',
                'extern/zlib/zlib.gyp:zlib',
            ],
            'include_dirs': ['<@(include_dirs)'],
            'direct_dependent_settings': {
                'include_dirs': ['<@(include_dirs)'],
            },
            'defines': ['IMAGE_EXPORTS'],
            'defines!': ['V8_DEPRECATION_WARNINGS=1'],
            'sources': ['<@(include_files)', '<@(source_files)'],
        },
    ],
}