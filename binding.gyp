{
    "targets": [{
      "target_name": "leveldown"
    # , "cflags": ['-g'] # embed debug info: node-gyp build -d
    , 'xcode_settings': {
      # 'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
      # 'CLANG_CXX_LIBRARY': 'libc++',
      # 'MACOSX_DEPLOYMENT_TARGET': '10.7',
      # 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
      # 'GCC_ENABLE_CPP_RTTI': 'YES',
      'OTHER_CFLAGS': [
        '-mmacosx-version-min=10.7', # only supports above node v7
        # '-fexceptions',
        # '-Wall',
        '-O3'
      ]
    }
    , "conditions": [
          # ['OS == "mac"',
          #   {
          #     'ccflags': [
          #             '-mmacosx-version-min=10.7',
          #             '-std=c++11',
          #             '-stdlib=libc++'
          #     ]
          #   }
          # ],
          ["OS == 'win'", {
              "defines": [
                  "_HAS_EXCEPTIONS=0"
              ]
            , "msvs_settings": {
                  "VCCLCompilerTool": {
                      "RuntimeTypeInfo": "false"
                    , "EnableFunctionLevelLinking": "true"
                    , "ExceptionHandling": "2"
                    , "DisableSpecificWarnings": [ "4355", "4530" ,"4267", "4244", "4506" ]
                  }
              }
          }]
        , ['OS == "linux"', {
              'cflags': [
              ]
            , 'cflags!': [ '-fno-tree-vrp' ]
          }]
        , ['OS == "android"', {
              'cflags': [ '-fPIC' ]
            , 'ldflags': [ '-fPIC' ]
            , 'cflags!': [
                  '-fno-tree-vrp'
                , '-fno-exceptions'
                , '-mfloat-abi=hard'
                , '-fPIE'
              ]
            , 'cflags_cc!': [ '-fno-exceptions' ]
            , 'ldflags!': [ '-fPIE' ]
          }]
        , ['target_arch == "arm"', {
              'cflags': [ '-mfloat-abi=hard'
              ]
          }]
        ]
      , "dependencies": [
            "<(module_root_dir)/deps/leveldb/leveldb.gyp:leveldb"
        ]
      , "include_dirs"  : [
            "<!(node -e \"require('nan')\")"
        ]
      , "sources": [
            "src/batch.cc"
          , "src/database.cc"
          , "src/iterator.cc"
          , "src/leveldown.cc"
        ]
    }]
}
