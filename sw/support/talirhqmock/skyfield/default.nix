{ lib, buildPythonPackage, fetchPypi, numpy, certifi }:
let
  sgp4 = buildPythonPackage rec {
    pname = "sgp4";
    version = "2.12";

    src = fetchPypi {
      inherit pname version;
      sha256 = "0dncp9i5b6afkg7f8mj9j0qzsp008b8v73yc0qkmizhpns7mvwvx";
    };

    doCheck = false;
  };
  jplephem = buildPythonPackage rec {
    pname = "jplephem";
    version = "2.14";

    src = fetchPypi {
      inherit pname version;
      sha256 = "1qy6i1q972ff1f6y9c9vyr5x10p3rndp1ch6ihax4gyinvmshsii";
    };
    doCheck = false; # fails due to some numpy import
    propagatedBuildInputs = [ numpy ];
  };
in
buildPythonPackage rec {
  pname = "skyfield";
  version = "1.26";

  src = fetchPypi {
    inherit pname version;
    sha256 = "132q7amdn41r47wj9rg28sal0j0bl26ji3v9y9hsvx1hay1ccpj8";
  };
  doCheck = false; # requires some assay package
  propagatedBuildInputs = [ certifi sgp4 jplephem ];
}
