{ stdenv, buildGoPackage, fetchgit, fetchhg, fetchbzr, fetchsvn }:
buildGoPackage rec {
  name = "talir_cmd_desk";
  goPackagePath = "github.com/MLAB-project/TALIR01/sw/talir_cmd_desk";
  src = ../../../talir_cmd_desk;
  goDeps = ./deps.nix;
  installPhase = ''
    mkdir -p $bin
    dir="$NIX_BUILD_TOP/go/bin"
    [ -e "$dir" ] && cp -r $dir $bin
    mkdir -p $bin/share/talir_cmd_desk/
    cp -r $src/static $bin/share/talir_cmd_desk
  '';
  meta = {
  };
}
