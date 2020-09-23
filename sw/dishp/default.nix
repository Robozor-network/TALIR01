let
 pkgs = import (builtins.fetchTarball {
                name = "nixpkgs-20.03";
                url = "https://github.com/NixOS/nixpkgs/archive/20.03.tar.gz";
                # Hash obtained using `nix-prefetch-url --unpack <url>`
                sha256 = "0182ys095dfx02vl2a20j1hz92dx3mfgz2a6fhn31bqlp1wa8hlq";
        }) {};
in
{ stdenv ? pkgs.stdenv }:
stdenv.mkDerivation {
    name = "dishp-test";
    src = [ ./. ];
    buildInputs = [ ];
    buildPhase = ''
    	make test
    '';
    /*
    doCheck = true;
    checkPhase = ''
    	${./aux/test.sh}
    '';
    */
    installPhase = ''
        install -m 555 bin/dishp -Dt $out/bin/
    '';
}
