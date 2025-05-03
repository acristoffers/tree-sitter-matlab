{
  "variables": {
    "is_arm%": "<!(node -e \"console.log(process.arch.startsWith('arm') || process.arch === 'arm64' ? 1 : 0)\")"
  },
  "targets": [
    {
      "target_name": "tree_sitter_matlab_binding",
      "dependencies": [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except",
      ],
      "include_dirs": [
        "src",
      ],
      "sources": [
        "bindings/node/binding.cc",
        "src/parser.c",
        # NOTE: if your language has an external scanner, add it here.
      ],
      "conditions": [
        ["OS!='win'", {
          "conditions": [
            ["is_arm==1", {
              "cflags_c": [
                "-std=c11",
                "-march=armv8-a",
                "-mtune=generic"
              ],
            }, { # not ARM
              "cflags_c": [
                "-std=c11"
              ],
            }],
          ],
        }, { # OS == "win"
          "cflags_c": [
            "/std:c11",
            "/utf-8",
          ],
        }],
      ],
    }
  ]
}
