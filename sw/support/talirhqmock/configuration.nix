{ config, pkgs, ... } @ args:
let
  testing = ({testing=false;} // args).testing;
in {
  nixpkgs.config = {
    allowUnfree = true;
  };

  imports = [
    ./moinmoin.nix
    ./runtriggers-service.nix
  ];

  networking = {
    hostName = "talirhq";
    useDHCP = false;
    firewall.allowedTCPPorts = [ 80 81 8003 ];
    hosts = { "127.0.0.1" = [ "talirhq.local" "localhost" ]; };
  } // (if (!testing) then {
  } else {});

  time.timeZone = "Europe/Prague";

  environment.systemPackages = with pkgs; [
    wget curl vim git mc msmtp
    (pkgs.callPackage ./controlmux {})
    (pkgs.python3.withPackages (pyPkgs: with pyPkgs; [
      numpy dateutil pytz
      (pkgs.callPackage ./skyfield { inherit buildPythonPackage fetchPypi numpy certifi; })
    ]))
  ];

  services.openssh.enable = true;

  users.users.root.openssh.authorizedKeys.keys = [
    "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC16lgFXOLvYcQsskaXe10H/oEzRouQdsF6OR5IPvHrZ2BMfyIUYwouyctRNUg0CtDwrDTisk2Mm+6DwQlQ6k1Nx84qBRhGqg3UBXZwXJ/Bew851A7MgSHPDoV2Ws+GYHEWl1RjzcLxiOpT7J6Bta68rENuLXBvVQmYDzXe75bXbONsXnW86z9muOUsa4g9eUXbRZWvacciXP5eCybLxif5jNSYzwUyfg/kHR/Dtv1Bc9eaXfnCEnMOBJlm0kbswsmKHMYAme1DI4pusJxF8rwtQ50J43H1ry8wA7zd0j8St4mHYjKNWidM6BXrfEQ30S1dtOMuPg4gj3qry3qNUz2Z povik@alice-nixos"
  ];

  users.users.povik = {
    isNormalUser = true;
    extraGroups = [ "wheel" ];
  };

  users.groups.shadow.members = [ config.services.nginx.user ];
  users.groups.runtriggers.members = [ config.services.nginx.user ];
  system.activationScripts.chgrpShadow = {
    text = ''
      chgrp shadow /etc/shadow
      chmod 640 /etc/shadow
    '';
    deps = [ "groups" ];
  };

  services.runtriggers = {
    listenAddr = "unix:/run/runtriggers.sock";
    enable = true;
  };

  services.moinmoin2 = {
    enable = true;
    webServer = "gunicorn";
    wikis.kapybara = {
      siteName = "TALIR Wiki";
      #webHost = "kapybara.ujf.cas.cz";
      webLocation = "/wiki";
      superUsers = [ "povik" ];
      frontPage = "FrontPage";
      extraConfig = ''
        from MoinMoin.auth import GivenAuth
        auth = [GivenAuth(env_var='HTTP_X_FORWARDED_USER', autocreate=True)]
      '';
    };
  };

  systemd.services.dishp = let
    controlmux = pkgs.callPackage ./controlmux {};
    dishp = pkgs.callPackage ../../dishp {};
    signal-piping-tools = pkgs.callPackage ./signal-piping-tools {};
    dishp-input = pkgs.writeShellScriptBin "dishp-input" ''
      ${dishp}/bin/dishp -s > /run/dishp-state-fifo
    '';
  in {
    enable = true;
    script = ''
      rm -f /run/dishp-* # clear sockets
      mkfifo /run/dishp-state-fifo
      ${signal-piping-tools}/bin/servestream -p 8003 <>/run/dishp-state-fifo  &
      PATH=${dishp-input}/bin:$PATH ${controlmux}/bin/controlmux -server -socket /run/dishp -pri auto,man,auto2 -timeout 5 dishp-input
    '';
    wantedBy = [ "multi-user.target" ];
    path = [ pkgs.bash ];
  };

  systemd.services.dishp_cmd_desk = let
    controlmux = pkgs.callPackage ./controlmux {};
    talir_cmd_desk = pkgs.callPackage ./talir_cmd_desk {};
    fetchlog = pkgs.writeShellScript "fetchlog" ''
      #!/bin/sh
      journalctl -f -u dishp.service -o cat
    '';
  in {
    enable = true;
    script = ''
      ${talir_cmd_desk}/bin/talir_cmd_desk -listen ":81" -static ${talir_cmd_desk}/share/talir_cmd_desk/static \
        -logProgram ${fetchlog} \
        ${controlmux}/bin/controlmux -socket /run/dishp-man -name "manual"
    '';
    wantedBy = [ "multi-user.target" ];
    path = [ pkgs.bash pkgs.systemd ];
  };

  security.pam.services.nginx = {
    unixAuth = true;
    text = ''
      account required pam_unix.so
      auth sufficient pam_unix.so likeauth try_first_pass
      auth required pam_deny.so
    '';
  };

  services.fcgiwrap.enable = true;

  services.nginx = {
    package = pkgs.nginx.override {
      modules = with pkgs.nginxModules; [ pam ];
    };
    enable = true;

    virtualHosts."talirhq" = {
      default = true;
      extraConfig = ''
        auth_pam "Please enter your TALIRHQ credentials";
        auth_pam_service_name "nginx";
      '';
      locations = {
        "/" = {
          extraConfig = ''
            auth_pam off;
            return 301 /wiki;
          '';
        };
        "/storage/" = { alias = "/storage/"; extraConfig=''
          autoindex on;
        '';};
        "/runtriggers" = {
          proxyPass = "http://unix:/run/runtriggers.sock";
          proxyWebsockets = true;
          extraConfig = ''
            client_max_body_size 200M;
            proxy_set_header X-Forwarded-User $remote_user;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
            proxy_set_header X-Forwarded-Host $host;
            proxy_set_header X-Forwarded-Server $host;
            proxy_set_header Accept-Encoding "";
            proxy_set_header Authorization "";
            proxy_redirect off;
            proxy_buffering off;
            chunked_transfer_encoding off;
            proxy_read_timeout 86400;
          '';
        };

        "/wiki" = {
          extraConfig = ''
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
            proxy_set_header X-Forwarded-Host $host;
            proxy_set_header X-Forwarded-Server $host;
            proxy_set_header X-Forwarded-User $remote_user;
            proxy_set_header Accept-Encoding "";
            proxy_set_header Authorization "";
            proxy_pass http://unix:/run/moin/kapybara/gunicorn.sock;
          '';
        };
      };
    };
  };

  system.stateVersion = "20.03";

  users.motd = ''
    Hello.
  '';
}
