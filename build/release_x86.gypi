{
    # release x86 props
    'target_defaults': {
        'configurations': {
            'Release_x86': {
                'abstract': 1,
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'Optimization': 0, # disabled
                        'PreprocessorDefinitions': [
                            'NDEBUG',
                        ], # PreprocessorDefinitions
                        'EnableIntrinsicFunctions': 'true',
                        'WholeProgramOptimization': 'true',
                        'FavorSizeOrSpeed': 1, # Speed
                    }, # VCCLCompilerTool
                    'VCLinkerTool': {
                        'AdditionalLibraryDirectories': [
                            '$(OutDir)', '<(JSX)/extern/boost/lib/x86', '<(JSX)/extern/v8/lib/x86/release',
                        ], # AdditionalLibraryDirectories
                    }, # VCLinkerTool
                }, # msvs_settings
            }, # Debug_x86
        }, # configurations
    }, # target_defaults
}