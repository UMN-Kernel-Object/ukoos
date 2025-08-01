{
  outputs =
    {
      self,
      flake-utils,
      nixpkgs,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      rec {
        devShells.default = pkgs.mkShell {
          inputsFrom = [
            packages.ukoos
          ];
          nativeBuildInputs = [
            pkgs.bear
            pkgs.dtc
            pkgs.qemu
          ];
          shellHook = ''
            export CC=riscv64-none-elf-gcc
            export STRIP=riscv64-none-elf-strip
          '';
        };

        packages = {
          default = packages.ukoos;

          ukoos = pkgs.stdenvNoCC.mkDerivation {
            pname = "ukoos";
            version = "git-${self.shortRev or self.dirtyShortRev or "unknown"}";

            src = ./.;
            nativeBuildInputs = [
              pkgs.getopt
              pkgs.pkgsCross.riscv64-embedded.stdenv.cc.bintools.bintools
              pkgs.pkgsCross.riscv64-embedded.stdenv.cc.cc
              pkgs.python3
            ];

            dontUnpack = true;
            configurePhase = ''
              runHook preConfigure

              mkdir build
              cd build
              bash $src/configure

              runHook postConfigure
            '';
            installPhase = ''
              runHook preInstall

              make install DESTDIR=$out

              runHook postInstall
            '';
          };

          fsbl-milkv-duos = pkgs.callPackage ./src/image-milkv-duos/fsbl.nix {
            inherit (packages) opensbi-milkv-duos u-boot-milkv-duos;
          };
          opensbi-milkv-duos = pkgs.pkgsCross.riscv64-musl.callPackage ./src/image-milkv-duos/opensbi.nix {
            inherit (packages) u-boot-milkv-duos;
          };
          u-boot-milkv-duos = pkgs.pkgsCross.riscv64-musl.callPackage ./src/image-milkv-duos/u-boot.nix { };
        };
      }
    );
}
