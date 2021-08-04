let
	nixpkgs-src = builtins.fetchTarball {
		url = "https://github.com/NixOS/nixpkgs/archive/0cebf41b6683bb13ce2b77bcb6ab1334477b5b29.tar.gz";
		sha256 = "0dxrfr0w5ksvpjwz0d2hy7x7dirnc2xk9nw1np3wr6kvdlzhs3ik";
 	};
in rec {
 	pkgs = import nixpkgs-src {};
 	libad9361-iio = pkgs.callPackage other/libad9361-iio.nix {};
 	py2-construct = pkgs.callPackage other/construct.nix {
 		inherit (pkgs.python2.pkgs) buildPythonPackage six pytest arrow;
 	};
 	additional = gnuradio: rec {
 		beesat-sdr = pkgs.callPackage gnuradio/beesat-sdr.nix { inherit gnuradio; };
 		gr-talir = pkgs.callPackage gnuradio/gr-talir.nix { inherit gnuradio; };
 		gr-lfast = pkgs.callPackage gnuradio/gr-lfast.nix { inherit gnuradio; };
 		gr-mesa = pkgs.callPackage gnuradio/gr-mesa.nix { inherit gnuradio gr-lfast; };
 		gr-sat = pkgs.callPackage gnuradio/gr-satellites.nix {
 			inherit gnuradio beesat-sdr;
 			inherit (pkgs.python2Packages) requests urllib3 chardet idna;
 			construct = py2-construct;
 		};
 		gr-iio = pkgs.callPackage gnuradio/gr-iio.nix {
 			inherit gnuradio libad9361-iio;
 		};
 		gr-fosphor = pkgs.callPackage gnuradio/gr-fosphor.nix {
 			inherit gnuradio;
			inherit (pkgs.python2Packages) python;
		};
 	};
	gnuradio = pkgs.callPackage ./gnuradio {
		inherit (pkgs.python2Packages) numpy scipy matplotlib Mako cheetah pygtk pyqt4 wxPython lxml CoreAudio pyopengl requiredPythonModules;
		fftw = pkgs.fftwFloat;
		uhd = null;
		ootBlocks = additional;
	};
	inspectrum = pkgs.callPackage ./other/inspectrum.nix {
		inherit gnuradio;
	};
	python3-with-packages = pkgs.python3.withPackages (pkgs: with pkgs; [ scipy numpy ]); # libiio ]);
}
