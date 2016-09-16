{
    'includes': ['../../common.gypi'],
    'target_defaults': {
        'direct_dependent_settings': {
            'include_dirs': ['.'],
        },
        'actions': [
            {
                'action_name': 'pnglibconf',
                'inputs': [
                    'scripts/pnglibconf.h.prebuilt',
                ],
                'outputs': [
                    'pnglibconf.h',
                ],
                'conditions': [
                    ['OS=="win"',
                       { 'action': ['copy', '<@(_inputs)', '<(_outputs)'], },
                       { 'action': ['cp', '<@(_inputs)', '<(_outputs)'], },
                    ],
                ],
                'msvs_cygwin_shell': 0,
            }, # pnglibconf
        ], # actions

        'sources': [
            'png.c',
            'pngerror.c',
            'pngget.c',
            'pngmem.c',
            'pngpread.c',
            'pngread.c',
            'pngrio.c',
            'pngrtran.c',
            'pngrutil.c',
            'pngset.c',
            'pngtest.c',
            'pngtrans.c',
            'pngwio.c',
            'pngwrite.c',
            'pngwtran.c',
            'pngwutil.c',
            'png.h',
            'pngconf.h',
            'pngpriv.h',
        ], # sources
    }, # target_defaults

    'targets': [
        {
            'target_name': 'libpng',
            'type': 'static_library',
            'msvs_guid': '52246DC8-A147-4177-8C8F-5B78573B6D2F',
       }, # target - libpng
    ], # targets
}
