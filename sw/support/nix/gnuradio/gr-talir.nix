{ stdenv, cmake, pkgconfig, boost, gnuradio,
 python, swig, git, cppunit, fftwFloat }:
stdenv.mkDerivation rec {
  name = "gr-talir";

  src = ../../../gr-talir; # sw/gr-talir

  nativeBuildInputs = [
    cmake
    pkgconfig
    swig
    git
    cppunit
    fftwFloat
  ];

  buildInputs = [
    boost gnuradio python
  ];

  enableParallelBuilding = true;
}
