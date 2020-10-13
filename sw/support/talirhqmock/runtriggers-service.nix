{ config, lib, pkgs, ... }:
with lib;

let
  pkg = pkgs.callPackage ./runtriggers {};
  cfg = config.services.runtriggers;
in
{
  options.services.runtriggers = with types; {
    enable = mkEnableOption "Runtriggers";

    listenAddr = mkOption {
      type = str;
      default = "localhost:8080";
      example = "";
    };
  };

  config = mkIf cfg.enable {
    environment.systemPackages = [ pkg ];

    users.groups.runtriggers = {};

    systemd.services.runtriggers = {
      script = ''
        PATH=/run/current-system/sw/bin/:$PATH ${pkg}/bin/runtriggers -debug -basepath /runtriggers -listen ${cfg.listenAddr}
      '';
      wantedBy = [ "multi-user.target" ];
      path = with pkgs; [ iproute coreutils curl git procps ];
    };
  };
}