# This file is subject to the following copyright notice: 
#
# Copyright (c) 2003-2021 Eelco Dolstra and the Nixpkgs/NixOS contributors
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

{ stdenv
, fetchFromGitHub
, makeWrapper
, writeText
# Dependencies documented @ https://gnuradio.org/doc/doxygen/build_guide.html
# => core dependencies
, cmake
, pkgconfig
, git
, boost
, cppunit
, fftw
# => python wrappers
# May be able to upgrade to swig3
, python
, swig2
, numpy
, scipy
, matplotlib
# => grc - the gnu radio companion
, Mako
, cheetah
, pygtk # Note: GR is migrating to Mako. Cheetah should be removed for GR3.8
# => gr-wavelet: collection of wavelet blocks
, gsl
# => gr-qtgui: the Qt-based GUI
, qt4
, qwt
, pyqt4
# => gr-wxgui: the Wx-based GUI
, wxPython
, lxml
# => gr-audio: audio subsystems (system/OS dependent)
, alsaLib   # linux   'audio-alsa'
, CoreAudio # darwin  'audio-osx'
# => uhd: the Ettus USRP Hardware Driver Interface
, uhd
# => gr-video-sdl: PAL and NTSC display
, SDL
# Other
, libusb1
, orc
, pyopengl
, xorg
, buildEnv
, requiredPythonModules
, ootBlocks ? (x: [])
}:
let
  base = stdenv.mkDerivation rec {
    pname = "gnuradio";
    version = "3.7.14.0";

    src = fetchFromGitHub {
      owner = "gnuradio";
      repo = "gnuradio";
      rev = "v${version}";
      sha256 = "1nh4f9dmygprlbqybd3j1byg9fsr6065n140mvc4b0v8qqygmhrc";
      fetchSubmodules = true;
    };

    nativeBuildInputs = [
      cmake
      pkgconfig
      git
      makeWrapper
      cppunit
      orc
    ];

    buildInputs = [
      boost
      fftw
      python
      swig2
      lxml
      qt4
      qwt
      SDL
      libusb1
      uhd
      gsl
    ] ++ stdenv.lib.optionals stdenv.isLinux  [ alsaLib   ]
      ++ stdenv.lib.optionals stdenv.isDarwin [ CoreAudio ];

    propagatedBuildInputs = [
      Mako
      cheetah
      numpy
      scipy
      matplotlib
      pyqt4
      pygtk
      wxPython
      pyopengl
    ];

    NIX_LDFLAGS = "-lpthread";

    enableParallelBuilding = true;

    postPatch = ''
      substituteInPlace \
          gr-fec/include/gnuradio/fec/polar_decoder_common.h \
          --replace BOOST_CONSTEXPR_OR_CONST const
    '';

    # Enables composition with nix-shell
    grcSetupHook = writeText "grcSetupHook.sh" ''
      addGRCBlocksPath() {
        addToSearchPath GRC_BLOCKS_PATH $1/share/gnuradio/grc/blocks
      }
      addEnvHooks "$targetOffset" addGRCBlocksPath
    '';

    setupHook = [ grcSetupHook ];

    # patch wxgui and pygtk check due to python importerror in a headless environment
    # wxgtk gui will be removed in GR3.8
    # c++11 hack may not be necessary anymore
    preConfigure = ''
      export NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -Wno-unused-variable ${stdenv.lib.optionalString (!stdenv.isDarwin) "-std=c++11"}"
      sed -i 's/.*wx\.version.*/set(WX_FOUND TRUE)/g' gr-wxgui/CMakeLists.txt
      sed -i 's/.*pygtk_version.*/set(PYGTK_FOUND TRUE)/g' grc/CMakeLists.txt
      find . -name "CMakeLists.txt" -exec sed -i '1iadd_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++11>)' "{}" ";"
    '';

    # Framework path needed for qwt6_qt4 but not qwt5
    cmakeFlags =
      stdenv.lib.optionals stdenv.isDarwin [ "-DCMAKE_FRAMEWORK_PATH=${qwt}/lib" ];

    # - Ensure we get an interactive backend for matplotlib. If not the gr_plot_*
    #   programs will not display anything. Yes, $MATPLOTLIBRC must point to the
    #   *dirname* where matplotlibrc is located, not the file itself.
    # - GNU Radio core is C++ but the user interface (GUI and API) is Python, so
    #   we must wrap the stuff in bin/.
    # Notes:
    # - May want to use makeWrapper instead of wrapProgram
    # - may want to change interpreter path on Python examples instead of wrapping
    # - see https://github.com/NixOS/nixpkgs/issues/22688 regarding use of --prefix / python.withPackages
    # - see https://github.com/NixOS/nixpkgs/issues/24693 regarding use of DYLD_FRAMEWORK_PATH on Darwin
    postInstall = ''
      printf "backend : Qt4Agg\n" > "$out/share/gnuradio/matplotlibrc"

      #for file in $(find $out/bin $out/share/gnuradio/examples -type f -executable); do
      #    wrapProgram "$file" \
      #        --prefix PYTHONPATH : $PYTHONPATH:$(toPythonPath "$out") \
      #        --set MATPLOTLIBRC "$out/share/gnuradio" \
      #        ${stdenv.lib.optionalString stdenv.isDarwin "--set DYLD_FRAMEWORK_PATH /System/Library/Frameworks"}
      #done
    '';

    meta = with stdenv.lib; {
      description = "Software Defined Radio (SDR) software";
      longDescription = ''
        GNU Radio is a free & open-source software development toolkit that
        provides signal processing blocks to implement software radios. It can be
        used with readily-available low-cost external RF hardware to create
        software-defined radios, or without hardware in a simulation-like
        environment. It is widely used in hobbyist, academic and commercial
        environments to support both wireless communications research and
        real-world radio systems.
      '';
      homepage = "https://www.gnuradio.org";
      license = licenses.gpl3;
      platforms = platforms.linux ++ platforms.darwin;
      maintainers = with maintainers; [ bjornfor fpletz ];
    };
  };
  additional = ootBlocks base;
  additionalList = builtins.attrValues additional;
in buildEnv rec {
  name = "gnuradio-composite";
  passthru = {
    inherit base;
  } // additional;
  paths = [ base xorg.libX11 ] ++ additionalList ++ requiredPythonModules (base.propagatedBuildInputs ++ base.buildInputs 
            ++ builtins.concatLists (map (x: x.propagatedBuildInputs) additionalList));
  ignoreCollisions = true;
  postBuild = ''
    . "${makeWrapper}/nix-support/setup-hook"

    if [ -L "$out/bin" ]; then
        unlink "$out/bin"
    fi
    mkdir -p "$out/bin"

    for path in ${stdenv.lib.concatStringsSep " " paths}; do
      if [ -d "$path/bin" ]; then
        cd "$path/bin"
        for prg in *; do
          if [ -f "$prg" ]; then
            rm -f "$out/bin/$prg"
            if [ -x "$prg" ]; then
              makeWrapper "$path/bin/$prg" "$out/bin/$prg" \
                --set MATPLOTLIBRC "${base}/share/gnuradio" \
                --set PYTHONHOME "$out" --set PYTHONNOUSERSITE "true" \
                --prefix LD_LIBRARY_PATH : "$out"/lib \
                --prefix GRC_BLOCKS_PATH : ${stdenv.lib.makeSearchPath "share/gnuradio/grc/blocks" additionalList}
            fi
          fi
        done
      fi
    done
  '';
}
