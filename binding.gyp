{
    'variables': {
        'include_files': [
            'include/image/jsx/aligned_allocator.hpp',
            'include/image/jsx/geometry.hpp',
            'include/image/jsx/threads.hpp',
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
        'include_dirs': ['include', '<!(node -e require(\'v8pp\'))'],
    },
    'targets': [
        {
            'target_name': 'image',
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
            'sources': ['<@(include_files)', '<@(source_files)'],
        },
    ],
}