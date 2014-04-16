{
    # debug x64 props
    'target_defaults': {
        'configurations': {
            'Debug64': {
                'abstract': 1,
                'msvs_configuration_platform': 'x64',
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'Optimization': 0, # disabled
                        'PreprocessorDefinitions': [
                            '_DEBUG',
                        ], # PreprocessorDefinitions
                        'BasicRuntimeChecks': 3, # EnableFastChecks
                        'RuntimeLibrary': 3, # MultiThreadedDebugDL
                    }, # VCCLCompilerTool
                    'VCLinkerTool': {
                        'AdditionalLibraryDirectories': [
                            '$(OutDir)', '<(JSX)/extern/boost/lib/x64', '<(JSX)/extern/v8/lib/x64/debug',
                        ], # AdditionalLibraryDirectories
                    }, # VCLinkerTool
                }, # msvs_settings
            }, # Debug_x86
        }, # configurations
    }, # target_defaults
}