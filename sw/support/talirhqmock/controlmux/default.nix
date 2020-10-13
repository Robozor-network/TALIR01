{ stdenv, buildGoPackage, fetchgit, fetchhg, fetchbzr, fetchsvn }:
buildGoPackage rec {
  name = "controlmux";
  goPackagePath = "github.com/MLAB-project/TALIR01/sw/controlmux";
  src = ../../../controlmux;
  goDeps = ./deps.nix;
  meta = {
  };
}
