{ stdenv, fetchFromGitHub, cmake, pkgconfig, boost, gnuradio,
 python, swig, git, cppunit, fftwFloat, gr-lfast }:
stdenv.mkDerivation rec {
  name = "gr-mesa";

  src = fetchFromGitHub {
    owner = "ghostop14";
    repo = "gr-mesa";
    rev = "fd81bdc8fc5ef40e00ad7b4c15d8c42f25597eec";
    sha256 = "0qvych0dhixsqn79r48wb3fl73ysgf37ki6g5m4w4hgw1gyzp1l2";
  };

  nativeBuildInputs = [
    cmake
    pkgconfig
    swig
    git
    cppunit
    gr-lfast
    fftwFloat
  ];

  buildInputs = [
    boost gnuradio python
  ];

  enableParallelBuilding = true;
}
