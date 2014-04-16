{
    'variables': {
        # should point to jsx folder. Also used in the included gypi files
        'JSX' : '../jsx-priv',
    }, # variables
    'includes': [
        'build/common.gypi',
        'build/debug_x86.gypi',
        'build/debug_x64.gypi',
        'build/release_x86.gypi',
        'build/release_x64.gypi',
    ], # includes

    'target_defaults': {
        'msvs_settings': {

        }, # msvs_settings

        'configurations': {
            'Debug_Win32': {
                'inherit_from' : ['Debug_x86'],
            }, # Debug_Win32

            'Debug_x64': {
                'inherit_from' : ['Debug64'],
            }, # Debug_x64

            'Release_Win32': {
                'inherit_from' : ['Release_x86'],
            }, # Release_Win32

            'Release_x64': {
                'inherit_from' : ['Release64'],
            }, # Release_x64
        }, # configurations

        'sources': [
            'src/image.hpp',
            'src/image.cpp',
        ], # sources
    }, # target_defaults

    'targets': [
        {
            'target_name': 'image',
            'type': 'static_library',
        }, # target - image
    ], # targets
}