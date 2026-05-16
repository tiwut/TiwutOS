#pragma once
#include "../api/types.h"
#include "../components/stl.cpp"

class AppContainer {
public:
  String name;
  String permissions;
  bool is_isolated;
  Vector<String> files;

  AppContainer() {}
  AppContainer(String n) {
    name = n;
    is_isolated = true;
    permissions = "LIMITED";
  }
};

class Driver {
public:
  String name;
  bool is_basic_fallback;
  int crash_count;
  bool disabled;

  Driver() {}
  Driver(String n, bool fb) {
    name = n;
    is_basic_fallback = fb;
    crash_count = 0;
    disabled = false;
  }
};

class SystemManager {
public:
  Vector<AppContainer> apps;
  Vector<Driver> drivers;
  String current_partition;
  String partition_fs_type;
  bool encryption_enabled;
  String encryption_algorithm;
  int otp;

  void Init() {
    drivers.init();
    current_partition = "USB_RAM_BOOT";
    partition_fs_type = "EXT4";
    encryption_enabled = true;
    encryption_algorithm = "AES-256";

    drivers.push_back(Driver("basic_vga", true));
    drivers.push_back(Driver("basic_net", true));
    drivers.push_back(Driver("ext_nvidia", false));
    drivers.push_back(Driver("ext_rtl8139", false));
    otp = 123456;
  }

  void ScanPartitions() {
    bool found = false;
    if (!found) {
      current_partition = "USER_PARTITION_SELECTED";
      partition_fs_type = "BTRFS";
    }
  }

  void HandleDriverCrash(String driver_name) {
    for (int i = 0; i < drivers.size(); i++) {
      if (drivers[i].name == driver_name) {
        if (drivers[i].is_basic_fallback)
          return;
        drivers[i].crash_count++;
        if (drivers[i].crash_count > 3) {
          drivers[i].disabled = true;
        } else {
        }
      }
    }
  }

  bool AuthorizeConfigMod(int input_otp) { return input_otp == otp; }

  void InstallApp(String appName) {
    AppContainer app(appName);
    app.files.push_back(String("/App/") + appName + "/main.nx");
    apps.push_back(app);
  }

  void RunApp(String appName) {}
};

SystemManager sys_mgr;
