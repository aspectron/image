{
    'variables': {
        'version': '1.0.1',
        'simd%': 0,             # Include SIMD extensions
        'arith_enc%': 1,        # Include arithmetic encoding support
        'arith_dec%': 1,        # Include arithmetic decoding support
        'jpeg7%': 0,            # Emulate libjpeg v7 API/ABI (this makes libmozjpeg backward incompatible with libjpeg v6b)
        'jpeg8%': 0,            # Emulate libjpeg v8 API/ABI (this makes libmozjpeg backward incompatible with libjpeg v6b)
        'mem_srcdst%': 1,       # Include in-memory source/destination manager functions when emulating the libjpeg v6b or v7 API/ABI
    },

    'targets': [
        {
            'target_name': 'mozjpeg',
            'type': 'static_library',
            'msvs_guid': '5FD70E21-369E-404F-A12A-185A279BD58B',
            'direct_dependent_settings': {
                'include_dirs': ['.'],
            },
            'actions': [
                {
                    'variables': {
                        'jpeg_lib_version': '62',
                        'conditions': [
                            ['jpeg7', { 'jpeg_lib_version': '70' }],
                            ['jpeg8', { 'jpeg_lib_version': '80' }],
                        ],
                    },
                    'action_name': 'Configure',
                    'inputs': ['configure.py', 'win/jconfig.h.in', 'win/config.h.in'],
                    'outputs': ['jconfig.h', 'config.h'],
                    'action': ['python', 'configure.py',
                        '--project=libmozjpeg', '--version=<(version)', '--jpeg_lib_version=<(jpeg_lib_version)', 
                        '--arith_enc=<(arith_enc)', '--arith_dec=<(arith_dec)', '--mem_srcdst=<(mem_srcdst)',
                    ],
                    'msvs_cygwin_shell': 0,
                },
            ],
            'sources': [
                'jcapimin.c', 'jcapistd.c', 'jccoefct.c', 'jccolor.c', 'jcdctmgr.c',
                'jchuff.c', 'jcinit.c', 'jcmainct.c', 'jcmarker.c', 'jcmaster.c',
                'jcomapi.c', 'jcparam.c', 'jcphuff.c', 'jcprepct.c', 'jcsample.c',
                'jctrans.c', 'jdapimin.c', 'jdapistd.c', 'jdatadst.c', 'jdatasrc.c',
                'jdcoefct.c', 'jdcolor.c', 'jddctmgr.c', 'jdhuff.c', 'jdinput.c',
                'jdmainct.c', 'jdmarker.c', 'jdmaster.c', 'jdmerge.c', 'jdphuff.c',
                'jdpostct.c', 'jdsample.c', 'jdtrans.c', 'jerror.c', 'jfdctflt.c',
                'jfdctfst.c', 'jfdctint.c', 'jidctflt.c', 'jidctfst.c', 'jidctint.c',
                'jidctred.c', 'jquant1.c', 'jquant2.c', 'jutils.c', 'jmemmgr.c', 'jmemnobs.c',
            ],
            'conditions': [
                ['arith_enc', {
                    'sources': ['jaricom.c', 'jcarith.c'],
                }],
                ['arith_dec', {
                    'sources': ['jaricom.c', 'jdarith.c'],
                }],
                ['simd', {
                    'defines': ['WITH_SIMD'],
#                    'dependencies': ['simd'],
                    'conditions': [
                        ['simd_x64', { 'sources': ['simd/jsimd_x86_64.c']}],
                        ['simd_x86', { 'sources': ['simd/jsimd_i386.c']}],
                    ],
                }, { 'sources': ['jsimd_none.c'] }],
            ],
        },
        #TODO { 'target_name': 'simd', }
    ],
}