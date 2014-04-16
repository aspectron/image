{
    # release x64 props
    'target_defaults': {
        'configurations': {
            'Release64': {
                'abstract': 1,
                'msvs_configuration_platform': 'x64',
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
                            '$(OutDir)', '<(JSX)/extern/boost/lib/x64', '<(JSX)/extern/v8/lib/x64/release',
                        ], # AdditionalLibraryDirectories
                    }, # VCLinkerTool
                }, # msvs_settings
            }, # Release64
        }, # configurations
    }, # target_defaults
}