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
    },
    'targets': [
        {
            'target_name': 'image',
            'type': 'shared_library',
            'msvs_guid': '5D7D0E6F-40FF-495F-8907-68D1C3C8E7F4',
            'dependencies': [
                '<(jsx)/jsx-lib.gyp:jsx-lib',
                '<(jsx)/extern/extern.gyp:*',
                'extern/libpng/libpng.gyp:libpng',
                'extern/mozjpeg/mozjpeg.gyp:mozjpeg',
            ],
            'include_dirs': ['include'],
            'direct_dependent_settings': {
                'include_dirs': ['include'],
            },
            'defines': ['IMAGE_EXPORTS'],
            'sources': ['<@(include_files)', '<@(source_files)'],
        },
    ],
}