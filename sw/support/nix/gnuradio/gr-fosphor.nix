{ stdenv, cmake, pkgconfig, boost, gnuradio,
 python, swig, libiio,
 bison, flex, cppunit,
 ocl-icd, freeglut, glfw3, opencl-headers, xorg, xlibsWrapper, libGLU
}:
stdenv.mkDerivation rec {
  name = "gnuradio-fosphor";

  src = builtins.fetchGit {
    url = "git://git.osmocom.org/gr-fosphor";
    rev = "7b6b9961bc2d9b84daeb42a5c8f8aeba293d207c";
  };

  nativeBuildInputs = [
    cmake
    pkgconfig
    bison
    flex
    swig
  ];

  buildInputs = [
    boost gnuradio python
    cppunit
    ocl-icd freeglut glfw3 opencl-headers xorg.libX11 xlibsWrapper libGLU
  ];

  enableParallelBuilding = true;
}
