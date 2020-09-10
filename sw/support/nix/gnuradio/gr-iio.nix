{ stdenv, fetchFromGitHub, cmake, pkgconfig, boost, gnuradio,
 python, swig, libad9361-iio, libiio,
 bison, flex }:
stdenv.mkDerivation rec {
  version = "0.3";
  pname = "gnuradio-iio";

  src = fetchFromGitHub {
    owner = "analogdevicesinc";
    repo = "gr-iio";
    rev = "c73b83a4d93720a5144ede5af9595a99307f203e";
    sha256 = "04kq8vngk6vv7mg1z90s6vifymbc8rk1n99m46m636291cg8h51w";
  };

  nativeBuildInputs = [
    cmake
    pkgconfig
    bison
    flex
    swig
  ];

  buildInputs = [
    boost gnuradio libiio libad9361-iio python
  ];

  enableParallelBuilding = true;
}
