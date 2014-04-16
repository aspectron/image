{
    # common properties for all Windows projects
    'target_defaults': {
        # 'variables': {
        #     'JSX' : '../jsx-priv',
        # }, # variables
        'conditions': [
            ['OS=="win"', {
                'msvs_configuration_attributes': {
                    'OutputDirectory':'<(JSX)bin/$(Configuration) $(PlatformName)/',
                    # intermediate directory as : '$(SolutionDir)obj/$(Configuration) $(PlatformName)/$(ProjectName)/''
                }, # msvs_configuration_attributes

                'msvs_settings':{
                    'VCCLCompilerTool': {
                        'AdditionalOptions': [
                            '/Zm392', '/MP',
                        ], # AdditionalOptions
                        'AdditionalIncludeDirectories': [
                            '<(JSX)/sdk/core/src', '<(JSX)/extern', '<(JSX)/extern/boost/include', 
                            '<(JSX)/extern/v8/include',
                        ], # AdditionalIncludeDirectories
                        'PreprocessorDefinitions': [
                            '_WIN32', '_WIN32_WINNT=0x0601', 'WIN32', 'NOMINMAX', '_CRT_SECURE_NO_DEPRECATE', 
                            '_CRT_SECURE_NO_WARNINGS', '_SCL_SECURE_NO_WARNINGS', 'V8_USE_UNSAFE_HANDLES', 
                            'V8_DISABLE_DEPRECATIONS',
                        ], # PreprocessorDefinitions
                        'WarningLevel': 3, # Level3
                        'DebugInformationFormat': 3, # ProgramDatabase
                        'DisableSpecificWarnings': [
                            '4275', '4351', '4503', '4510', '4512', '4610',
                        ], # DisableSpecificWarnings
                    }, # VCCLCompilerTool

                    'VCLinkerTool': {
                        'AdditionalDependencies': [
                            'jsx.lib', 'v8.lib', 
                        ], # AdditionalDependencies
                        'GenerateDebugInformation': 'true',
                        'AdditionalOptions': [
                            '/IGNORE:4099',
                        ], # AdditionalOptions
                        'AdditionalLibraryDirectories': [
                            '<(JSX)/bin/$(Configuration) $(Platform)',
                        ], # AdditionalLibraryDirectories
                    }, # VCLinkerTool
                }, # msvs_settings
            }], # OS=="win"
        ], # conditions
    }, # target_defaults
}
