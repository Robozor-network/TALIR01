{ stdenv, fetchFromGitHub
, cmake, libiio, flex, bison, libxml2, python
}:

stdenv.mkDerivation rec {
  name = "libad9361-iio-${version}";
  version = "0.2";

  src = fetchFromGitHub {
    owner  = "analogdevicesinc";
    repo   = "libad9361-iio";
    rev    = "v${version}";
    sha256 = "1psxx8ssakxjmfgmm4c877nbg15ylscncy4x1d7gj4ni35chb2km";
  };

  nativeBuildInputs = [ cmake flex bison ];
  buildInputs = [ libxml2 libiio ];

  postInstall = ''
    #mkdir -p $python/lib/${python.libPrefix}/site-packages/
    #touch $python/lib/${python.libPrefix}/site-packages/
    #cp ../bindings/python/iio.py $python/lib/${python.libPrefix}/site-packages/

    #substitute ../bindings/python/iio.py $python/lib/${python.libPrefix}/site-packages/iio.py \
    #  --replace 'libiio.so.0' $lib/lib/libiio.so.0
  '';
}
