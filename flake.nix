{
  inputs.u-boot-milkv-duos = {
    url = "github:UMN-Kernel-Object/u-boot/milkv-duos-sd";
    inputs = {
      flake-utils.follows = "flake-utils";
      nixpkgs.follows = "nixpkgs";
    };
  };
  outputs =
    {
      self,
      flake-utils,
      nixpkgs,
      u-boot-milkv-duos,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        version = "git-${self.shortRev or self.dirtyShortRev or "unknown"}";

        ukoos =
          arch: target:
          pkgs.stdenvNoCC.mkDerivation {
            pname = "ukoos-${arch}-${target}";
            inherit version;

            src = ./.;
            nativeBuildInputs = [
              pkgs.getopt
              pkgs.mdbook
              pkgs.perl
              pkgs.pkgsCross.riscv64-embedded.stdenv.cc.bintools.bintools
              pkgs.pkgsCross.riscv64-embedded.stdenv.cc.cc
              pkgs.python3
            ];

            dontUnpack = true;
            configurePhase = ''
              runHook preConfigure

              mkdir build
              cd build
              bash $src/configure --arch ${arch} --target ${target}

              runHook postConfigure
            '';
            installPhase = ''
              runHook preInstall

              make install DESTDIR=$out

              runHook postInstall
            '';
          };
      in
      rec {
        devShells.default = pkgs.mkShell {
          inputsFrom = [
            packages.ukoos-milkv-duos
          ];
          nativeBuildInputs = [
            pkgs.bear
            pkgs.dtc
            pkgs.minicom
            pkgs.qemu
          ];
          shellHook = ''
            export CC=riscv64-none-elf-gcc
            export STRIP=riscv64-none-elf-strip
          '';
        };

        packages = {
          default = packages.ukoos-milkv-duos;

          ukoos-milkv-duos = ukoos "riscv64" "milkv-duos";
          ukoos-milkv-jupiter = ukoos "riscv64" "milkv-jupiter";
          ukoos-qemu-riscv64 = ukoos "riscv64" "qemu-riscv64";

          dev-image-milkv-duos = pkgs.callPackage ./src/image-milkv-duos {
            dev = true;
            inherit (packages) ukoos-milkv-duos u-boot-milkv-duos;
          };
          image-milkv-duos = pkgs.callPackage ./src/image-milkv-duos {
            dev = false;
            inherit (packages) ukoos-milkv-duos u-boot-milkv-duos;
          };
          u-boot-milkv-duos = u-boot-milkv-duos.packages.${system}.default;
          inherit (packages.image-milkv-duos) fsbl-milkv-duos opensbi-milkv-duos;
        };
      }
    );
}
