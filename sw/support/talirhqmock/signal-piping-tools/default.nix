{ stdenv, fetchFromGitHub,
  gnuradio, libusb1, cfitsio
}:

stdenv.mkDerivation rec {
  name = "signal-piping-tools";

  src = fetchFromGitHub {
    owner  = "MLAB-project";
    repo   = "signal-piping-tools";
    rev    = "e34673202329397de0947fa825297d20f0bdb758";
    sha256 = "0ycmn1yy7p3v32ygjqblbxgidxq7pwrj04qb5n5y0kdpnxs43kw0";
  };

  # gnuradio is needed for volk
  buildInputs = [ gnuradio libusb1 cfitsio ];

  installPhase = ''
    mkdir -p $out/bin
    cp fitsread servecmd servestream ptee hpsdrrecv x_fir_dec sdr-widget $out/bin/.
  '';
}
