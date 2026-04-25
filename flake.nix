# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

{
  inputs.u-boot-milkv-duos-sd = {
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
      u-boot-milkv-duos-sd,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        version = "g${self.shortRev or self.dirtyShortRev or "unknown"}";

        all-targets = [
          "milkv-duos"
          "milkv-jupiter"
          "qemu-riscv64"
        ];
        all-image-targets = [
          "milkv-duos-sd"
        ];

        ukoos =
          target:
          pkgs.stdenvNoCC.mkDerivation {
            pname = "ukoos-${target}";
            inherit version;

            src = ./.;
            nativeBuildInputs = [
              pkgs.getopt
              pkgs.mdbook
              pkgs.perl
              pkgs.pkgsCross.riscv64-embedded.stdenv.cc.bintools.bintools
              pkgs.pkgsCross.riscv64-embedded.stdenv.cc.cc
              pkgs.pkgsCross.riscv64-embedded.buildPackages.llvmPackages.clang-unwrapped
              pkgs.pkgsCross.riscv64-embedded.buildPackages.llvmPackages.lld
              pkgs.python3
              pkgs.watchexec
            ];

            dontUnpack = true;
            configurePhase = ''
              runHook preConfigure

              mkdir build
              cd build
              GDB=false bash $src/configure --target ${target}

              runHook postConfigure
            '';
            installPhase = ''
              runHook preInstall

              make install DESTDIR=$out

              runHook postInstall
            '';
          };
        ukoos-packages = builtins.listToAttrs (
          builtins.map (target: {
            name = "ukoos/${target}";
            value = ukoos target;
          }) all-targets
        );
      in
      rec {
        checks = {
          build-ukoos-for-all-targets = pkgs.linkFarm "ukoos-all-targets" (
            builtins.map (target: {
              name = target;
              path = packages."ukoos/${target}";
            }) all-targets
          );
          build-images-for-all-targets = packages.all-images;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [
            packages."ukoos/qemu-riscv64"
          ];
          nativeBuildInputs = [
            pkgs.bear
            (pkgs.runCommand "busybox" { } "install -Dt $out/bin ${pkgs.pkgsStatic.busybox}/bin/busybox")
            pkgs.clang-tools
            pkgs.dtc
            pkgs.gdb
            pkgs.minicom
            pkgs.nixfmt
            pkgs.qemu
            pkgs.reuse
            pkgs.shellcheck
            pkgs.tio
            pkgs.watchexec
          ];
          shellHook = ''
            export CC=riscv64-none-elf-gcc
            export OBJDUMP=riscv64-none-elf-objdump
            export STRIP=riscv64-none-elf-strip
          '';
        };

        formatter = pkgs.nixfmt-tree;

        packages = ukoos-packages // {
          default = packages."ukoos/milkv-duos";

          all-images = pkgs.linkFarm "ukoos-images" (
            builtins.concatMap (image-target: [
              {
                name = "dev-image-${image-target}.img";
                path = "${packages."dev-image/${image-target}"}/ukoos.img";
              }
              {
                name = "image-${image-target}.img";
                path = "${packages."image/${image-target}"}/ukoos.img";
              }
            ]) all-image-targets
          );

          "dev-image/milkv-duos-sd" = pkgs.callPackage ./src/image-milkv-duos-sd {
            dev = true;
            u-boot-milkv-duos-sd = u-boot-milkv-duos-sd.packages.${system}.default;
            ukoos-milkv-duos = packages."ukoos/milkv-duos";
          };
          "image/milkv-duos-sd" = pkgs.callPackage ./src/image-milkv-duos-sd {
            dev = false;
            u-boot-milkv-duos-sd = u-boot-milkv-duos-sd.packages.${system}.default;
            ukoos-milkv-duos = packages."ukoos/milkv-duos";
          };
        };
      }
    );
}
