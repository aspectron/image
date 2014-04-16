{
    # debug x86 props
    'target_defaults': {
        'configurations': {
            'Debug_x86': {
                'abstract': 1,
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
                            '$(OutDir)', '<(JSX)/extern/boost/lib/x86', '<(JSX)/extern/v8/lib/x86/debug',
                        ], # AdditionalLibraryDirectories
                    }, # VCLinkerTool
                }, # msvs_settings
            }, # Debug_x86
        }, # configurations
    }, # target_defaults
}