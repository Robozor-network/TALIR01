{ stdenv, buildGoPackage, fetchgit, fetchhg, fetchbzr, fetchsvn }:
buildGoPackage rec {
  name = "runtriggers-unstable-${version}";
  version = "2020-09-17";
  rev = "b439f25a4714c128a2d6f11673a147badc96a77a";

  goPackagePath = "github.com/povik/runtriggers";

  src = /home/povik/repos/runtriggers;

  goDeps = ./deps.nix;

  installPhase = ''
    mkdir -p $bin
    dir="$NIX_BUILD_TOP/go/bin"
    [ -e "$dir" ] && cp -r $dir $bin
    cp -r $src/static $bin/bin
    cp -r $src/templates $bin/bin
  '';

  # TODO: add metadata https://nixos.org/nixpkgs/manual/#sec-standard-meta-attributes
  meta = {
  };
}