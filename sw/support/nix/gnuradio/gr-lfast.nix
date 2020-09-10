{ stdenv, fetchFromGitHub, cmake, pkgconfig, boost, gnuradio,
 python, swig, git, cppunit, fftwFloat }:
stdenv.mkDerivation rec {
  name = "gr-mesa";

  src = fetchFromGitHub {
    owner = "ghostop14";
    repo = "gr-lfast";
    rev = "832d72a6fe40c25c797e5eab26f531a332492ce5";
    sha256 = "14gr6lsq2wynylfpsczxm2410bsmyd60n1fkkg1mll5sq6317y65";
  };

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
