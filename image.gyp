{
    'targets': [
        {
            'target_name': 'image',
            'type': 'shared_library',
            'msvs_guid': '5D7D0E6F-40FF-495F-8907-68D1C3C8E7F4',
            'dependencies': [
                '<(jsx)/sdk/core/core.gyp:core',
                '<(jsx)/extern/extern.gyp:*',
                '<(jsx)/extern/zlib/zlib.gyp:zlib',
                '<(jsx)/extern/libpng/libpng.gyp:libpng',
            ],
            'direct_dependent_settings': {
                'include_dirs': ['src'],
            },
            'export_dependent_settings': [
                '<(jsx)/extern/zlib/zlib.gyp:zlib',
                '<(jsx)/extern/libpng/libpng.gyp:libpng',
            ],
            'defines': ['IMAGE_EXPORTS'],
            'sources': [
                'src/image.hpp',
                'src/image.cpp',
                'src/bitmap.cpp',
                'src/bitmap.hpp',
                'src/quantizer.cpp',
                'src/quantizer.hpp',
            ],
        },
    ],
}