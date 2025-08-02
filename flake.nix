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
            packages.ukoos-riscv64
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
          default = packages.ukoos-riscv64;

          ukoos-riscv64 = pkgs.stdenvNoCC.mkDerivation {
            pname = "ukoos-riscv64";
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

          image-milkv-duos = pkgs.callPackage ./src/image-milkv-duos {
            inherit (packages) ukoos-riscv64;
          };
          inherit (packages.image-milkv-duos) fsbl-milkv-duos opensbi-milkv-duos u-boot-milkv-duos;
        };
      }
    );
}
