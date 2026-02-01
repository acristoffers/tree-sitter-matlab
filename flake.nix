{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        lib = pkgs.lib;
        libSuffix = if pkgs.stdenv.isDarwin then "dylib" else "so";

        mkTreeSitterMatlab = { abiVersion, pnameSuffix }:
          pkgs.stdenv.mkDerivation {
            pname = "tree-sitter-matlab${pnameSuffix}";
            version = "1.2.13";
            src = ./.;

            nativeBuildInputs = [
              pkgs.tree-sitter
              pkgs.nodejs
            ];

            buildInputs = [
              pkgs.stdenv.cc
            ];

            doCheck = true;
            checkPhase = ''
              runHook preCheck

              HOME=$TMPDIR

              workDir="$TMPDIR/tree-sitter-matlab-${abiVersion}"
              mkdir -p "$workDir"
              cp -R . "$workDir"
              chmod -R u+rwX "$workDir"

              pushd "$workDir" >/dev/null
              tree-sitter test
              popd >/dev/null

              runHook postCheck
            '';

            buildPhase = ''
              runHook preBuild

              HOME=$TMPDIR

              workDir="$TMPDIR/tree-sitter-matlab-${abiVersion}"
              mkdir -p "$workDir"
              cp -R . "$workDir"
              chmod -R u+rwX "$workDir"

              pushd "$workDir" >/dev/null
              tree-sitter generate --abi ${abiVersion}
              mkdir -p "$out/lib"
              tree-sitter build --output "$out/lib/tree-sitter-matlab.${libSuffix}"
              popd >/dev/null

              runHook postBuild
            '';

            installPhase = "true";

            meta = with lib; {
              description = "MATLAB tree-sitter parser (ABI ${abiVersion})";
              homepage = "https://github.com/acristoffers/tree-sitter-matlab";
              license = licenses.mit;
              platforms = platforms.unix;
            };
          };

        mkQueriesPackage = { editor, srcDir, installDir }:
          pkgs.stdenvNoCC.mkDerivation {
            pname = "tree-sitter-matlab-queries-${editor}";
            version = "1.2.13";
            src = ./.;

            dontBuild = true;
            dontConfigure = true;

            installPhase = ''
              runHook preInstall
              mkdir -p "$out/${installDir}/matlab"
              cp -R ${srcDir}/*.scm "$out/${installDir}/matlab/"
              runHook postInstall
            '';

            meta = with lib; {
              description = "Editor queries for tree-sitter-matlab (${editor})";
              homepage = "https://github.com/acristoffers/tree-sitter-matlab";
              license = licenses.mit;
              platforms = platforms.unix;
            };
          };

        mkTreeSitterMatlabWasm = pkgs.stdenv.mkDerivation {
          pname = "tree-sitter-matlab-wasm";
          version = "1.2.13";
          src = ./.;

          nativeBuildInputs = [
            pkgs.tree-sitter
            pkgs.nodejs
            pkgs.emscripten
          ];

          buildPhase = ''
            runHook preBuild

            HOME=$TMPDIR

            workDir="$TMPDIR/tree-sitter-matlab-wasm"
            mkdir -p "$workDir"
            cp -R . "$workDir"
            chmod -R u+rwX "$workDir"

            pushd "$workDir" >/dev/null
            tree-sitter generate
            mkdir -p "$out/lib"
            tree-sitter build --wasm --output "$out/lib/tree-sitter-matlab.wasm"
            popd >/dev/null

            runHook postBuild
          '';

          installPhase = "true";

          meta = with lib; {
            description = "MATLAB tree-sitter parser (WASM)";
            homepage = "https://github.com/acristoffers/tree-sitter-matlab";
            license = licenses.mit;
            platforms = platforms.unix;
          };
        };
      in
      {
        packages = {
          tree-sitter-matlab = mkTreeSitterMatlab {
            abiVersion = "15";
            pnameSuffix = "";
          };
          tree-sitter-matlab-abi-14 = mkTreeSitterMatlab {
            abiVersion = "14";
            pnameSuffix = "-abi-14";
          };
          tree-sitter-matlab-wasm = mkTreeSitterMatlabWasm;

          tree-sitter-matlab-queries-neovim = mkQueriesPackage {
            editor = "neovim";
            srcDir = "queries/neovim";
            installDir = "share/nvim/site/queries";
          };
          tree-sitter-matlab-queries-helix = mkQueriesPackage {
            editor = "helix";
            srcDir = "queries/helix";
            installDir = "share/helix/runtime/queries";
          };
          tree-sitter-matlab-queries-emacs = mkQueriesPackage {
            editor = "emacs";
            srcDir = "queries/emacs";
            installDir = "share/emacs/site-lisp/tree-sitter/queries";
          };

          default = self.packages.${system}.tree-sitter-matlab;
        };

        apps = {
          fuzzy-test = {
            type = "app";
            program = "${pkgs.writeShellApplication {
              name = "tree-sitter-matlab-fuzzy-test";
              runtimeInputs = with pkgs; [
                clang
                coreutils
                gnugrep
                jq
                nodejs
                pkg-config
                tree-sitter
              ];
              text = ''
                time_arg="''${1:-60}"
                work_dir="$(mktemp -d)"
                export HOME="$work_dir"
                export PKG_CONFIG_PATH="${pkgs.tree-sitter}/lib/pkgconfig''${PKG_CONFIG_PATH:+:}"
                export CFLAGS="-I${pkgs.tree-sitter}/include ''${CFLAGS:-}"
                cp -R ${./.} "$work_dir/repo"
                chmod -R u+rwX "$work_dir/repo"
                cd "$work_dir/repo"
                exec ./fuzzy.sh matlab "$time_arg" c
              '';
            }}/bin/tree-sitter-matlab-fuzzy-test";
          };
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            busybox
            cargo
            clang
            cmake
            fzf
            git
            gnumake
            jq
            nodejs
            pkg-config
            python3
            tree-sitter
          ];
        };

        formatter = pkgs.nixpkgs-fmt;
      }
    );
}
