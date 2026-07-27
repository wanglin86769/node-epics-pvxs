{
  "targets": [
    {
      "target_name": "pvxs",
      "sources": [
        "src/addon.cpp",
        "src/common/errors.cpp",
        "src/common/tsfn.cpp",
        "src/data/init.cpp",
        "src/data/typedef.cpp",
        "src/data/value_convert.cpp",
        "src/data/value_wrap.cpp",
        "src/data/put_payload.cpp",
        "src/client/events.cpp",
        "src/client/config.cpp",
        "src/client/worker.cpp",
        "src/client/subscription.cpp",
        "src/client/discover.cpp",
        "src/client/context.cpp",
        "src/client/init.cpp",
        "src/server/init.cpp",
        "src/server/exec_op.cpp",
        "src/server/sharedpv.cpp",
        "src/server/static_source.cpp",
        "src/server/server.cpp"
      ],
      "include_dirs": [
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "src",
        "<(pvxs_root)/include",
        "<(epics_base)/include"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_CPP_EXCEPTIONS"
      ],
      "cflags!": [ "-fno-exceptions", "-fno-rtti" ],
      "cflags_cc!": [ "-fno-exceptions", "-fno-rtti" ],
      "conditions": [
        ["OS=='win'", {
          "defines": [
            "EPICS_CALL_DLL",
            "_HAS_EXCEPTIONS=1"
          ],
          "defines!": [
            "_HAS_EXCEPTIONS=0"
          ],
          "include_dirs": [
            '<(epics_base)/include/compiler/msvc',
            '<(epics_base)/include/os/WIN32'
          ],
          "libraries": [
            '<(pvxs_lib_dir)/pvxs.lib',
            '<(epics_base_lib_dir)/Com.lib',
            '<(libevent_lib_dir)/event_core.lib'
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "RuntimeTypeInfo": "true",
              "LanguageStandard": "stdcpp17",
              "AdditionalOptions": [
                "/GR",
                "%(AdditionalOptions)"
              ]
            }
          },
          "configurations": {
            "Release": {
              "msvs_settings": {
                "VCCLCompilerTool": {
                  "RuntimeTypeInfo": "true"
                }
              }
            }
          }
        }],
        ["OS=='linux'", {
          "include_dirs": [
            '<(epics_base)/include/compiler/gcc',
            '<(epics_base)/include/os/Linux'
          ],
          "library_dirs": [
            '<(pvxs_lib_dir)',
            '<(epics_base_lib_dir)',
            '<(libevent_lib_dir)'
          ],
          "libraries": [
            "-lpvxs",
            "-lCom",
            "-levent_core"
          ],
          "cflags_cc": [
            "-std=c++17"
          ]
        }],
        ["OS=='mac'", {
          "include_dirs": [
            '<(epics_base)/include/compiler/clang',
            '<(epics_base)/include/os/Darwin'
          ],
          "library_dirs": [
            '<(pvxs_lib_dir)',
            '<(epics_base_lib_dir)',
            '<(libevent_lib_dir)'
          ],
          "libraries": [
            "-lpvxs",
            "-lCom",
            "-levent_core"
          ],
          "cflags_cc": [
            "-std=c++17"
          ],
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "GCC_ENABLE_CPP_RTTI": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15"
          }
        }]
      ]
    }
  ],
  "variables": {
    "pvxs_root%": "<!(node scripts/resolve-epics-paths.js pvxs_root)",
    "epics_base%": "<!(node scripts/resolve-epics-paths.js epics_base)",
    "pvxs_lib_dir%": "<!(node scripts/resolve-epics-paths.js pvxs_lib_dir)",
    "epics_base_lib_dir%": "<!(node scripts/resolve-epics-paths.js epics_base_lib_dir)",
    "libevent_lib_dir%": "<!(node scripts/resolve-epics-paths.js libevent_lib_dir)"
  }
}
