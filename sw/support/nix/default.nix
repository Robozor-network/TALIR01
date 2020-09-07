let
	nixpkgs-src = builtins.fetchTarball {
		url = "https://github.com/NixOS/nixpkgs/archive/51d115ac89d676345b05a0694b23bd2691bf708a.tar.gz";
		sha256 = "1gfjaa25nq4vprs13h30wasjxh79i67jj28v54lkj4ilqjhgh2rs";
 	};
in rec {
 	pkgs = import nixpkgs-src {};
 	additional = gnuradio: builtins.attrValues rec {
 		beesat-sdr = pkgs.callPackage ./gnuradio/beesat-sdr.nix { inherit gnuradio; };
 		gr-sat = pkgs.callPackage ./gnuradio/gr-satellites.nix {
 			inherit gnuradio beesat-sdr;
 			inherit (pkgs.python2Packages) construct requests urllib3 chardet idna;
 		};
 	};
	gnuradio = pkgs.callPackage ./gnuradio {
		inherit (pkgs.python2Packages) numpy scipy matplotlib Mako cheetah pygtk pyqt4 wxPython lxml CoreAudio pyopengl requiredPythonModules;
		fftw = pkgs.fftwFloat;
		uhd = null;
		ootBlocks = additional;
	};
}
