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
    buildInputs = [ pkgs.gnuplot pkgs.jq ];
    buildPhase = ''
        make test
    '';
    doCheck = true;
    outputs = ["out" "test"];
    checkPhase = ''
        mkdir -p $test/logs
        function parse() {
            jq --raw-output '[.t,.alt_user,.az_user] | join(" ")'  
        }
        { bin/dishp -sn | parse; } > $test/logs/slow << EOF
        set azspeed 8500
        set altspeed 6000
        move 10000 2000
        waitidle
        move 0 0
        waitidle
        EOF
        { bin/dishp -sn | parse; } > $test/logs/fast << EOF
        set azspeed 17000
        set altspeed 12000
        move 10000 2000
        waitidle
        move 0 0
        waitidle
        EOF
        gnuplot <<- EOF
        set term png
        set output "$test/plot-alt.png"
        plot "$test/logs/slow" using 1:2 title "slow", "$test/logs/fast" using 1:2 title "fast"
        set output "$test/plot-az.png"
        plot "$test/logs/slow" using 1:3 title "slow", "$test/logs/fast" using 1:3 title "fast"
        EOF
    '';
    installPhase = ''
        install -m 555 bin/dishp -Dt $out/bin/
    '';
}
