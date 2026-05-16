#pragma once
#include "../components/stl.cpp"
#include "system_manager.cpp"

class NexusNative {
public:
  Map<String, String> vars;
  Map<String, Vector<String>> funcs;

  void Init() {
    vars.init();
    funcs.init();
    vars.set("OS", "TiwutOS-v3.1");
    vars.set("VER", "3.1-x64");
    vars.set("ENGINE", "NATIVE-OS");
  }

  String CallSystemAPI(String mod, String func, String arg) {
    if (mod == "sys") {
      if (func == "os")
        return vars.get("OS");
      if (func == "auth") {
        int otp_in = 0;
        for (int i = 0; i < arg.len; i++)
          otp_in = otp_in * 10 + (arg.data[i] - '0');
        if (sys_mgr.AuthorizeConfigMod(otp_in))
          return "AUTH_SUCCESS";
        return "AUTH_FAILED";
      }
    }
    if (mod == "window") {
      if (func == "create") {
        return "WINDOW_CREATED";
      }
      if (func == "draw_rect") {
        return "RECT_DRAWN";
      }
    }
    if (mod == "app") {
      if (func == "install") {
        sys_mgr.InstallApp(arg);
        return "INSTALLED";
      }
    }
    if (mod == "driver") {
      if (func == "import") {
        sys_mgr.drivers.push_back(Driver(arg, false));
        return "DRIVER_IMPORTED";
      }
    }
    return "NIL";
  }

  void RunScript(String script) {
    char line[256];
    int li = 0;
    for (int i = 0; i < script.len; i++) {
      if (script.data[i] == '\n' || script.data[i] == '\r') {
        if (li > 0) {
          line[li] = 0;
          RunLine(String(line));
          li = 0;
        }
      } else {
        if (li < 255)
          line[li++] = script.data[i];
      }
    }
    if (li > 0) {
      line[li] = 0;
      RunLine(String(line));
    }
  }

  void RunLine(String line) {
    if (line.len == 0)
      return;

    if (line.len > 5 && line.data[0] == '+' && line.data[1] == 'a' &&
        line.data[2] == 'd' && line.data[3] == 'd' && line.data[4] == ' ') {
      String lib = line.substring(5, line.len - 5);
      vars.set(String("NATIVE_LIB_") + lib, "LOADED");
      return;
    }
    if (line.len > 7 && line.data[0] == 'i' && line.data[1] == 'm' &&
        line.data[2] == 'p' && line.data[3] == 'o' && line.data[4] == 'r' &&
        line.data[5] == 't' && line.data[6] == ' ') {
      String mod = line.substring(7, line.len - 7);
      vars.set(String("IMPORT_") + mod, "ACTIVE");
      return;
    }

    if (line.len > 4 && line.data[0] == 's' && line.data[1] == 'e' &&
        line.data[2] == 't' && line.data[3] == ' ') {
      int eq = -1;
      for (int i = 4; i < line.len; i++)
        if (line.data[i] == '=') {
          eq = i;
          break;
        }
      if (eq != -1) {
        String name = line.substring(4, eq - 4);
        String val = line.substring(eq + 1, line.len - eq - 1);
        vars.set(name, val);
      }
      return;
    }

    int dot = -1;
    int lp = -1;
    int rp = -1;
    for (int i = 0; i < line.len; i++) {
      if (line.data[i] == '.')
        dot = i;
      if (line.data[i] == '(')
        lp = i;
      if (line.data[i] == ')')
        rp = i;
    }

    if (dot != -1 && lp != -1 && rp != -1 && dot < lp && lp < rp) {
      String mod = line.substring(0, dot);
      String func = line.substring(dot + 1, lp - dot - 1);
      String arg = line.substring(lp + 1, rp - lp - 1);
      CallSystemAPI(mod, func, arg);
    }
  }
};

NexusNative nexus_engine;
