char browser_content[4096] = "Welcome to the Native Real Internet!\\nType a URL (e.g. google.com) and press Enter.\\nTiwutOS will perform a native UDP DNS lookup!";

void DrawBrowser(Window* w, int cx, int cy) {
    Screen::Rect(w->x, w->y+30, w->w, 30, 0xDDDDDD);
    Screen::Rect(w->x+5, w->y+35, w->w-10, 20, 0xFFFFFF);
    DrawText(w->x+10, w->y+41, w->buffer, 0x000000);
    if(w->active) DrawText(w->x+10 + (Utils::strlen(w->buffer)*9), w->y+41, "|", 0);
    
    int content_y = cy + 30;
    Screen::Rect(w->x, content_y, w->w, w->h-60, 0xFFFFFF);
    
    int ly = content_y + 5;
    char* p = browser_content; char line[128]; int li=0;
    while(*p) {
        if(*p=='\n' || li>=(w->w/8 - 2)) { 
            line[li]=0; DrawText(cx, ly, line, 0x000000); ly+=12; li=0; 
            if(*p=='\n') p++; 
            if(ly > w->y + w->h - 15) break;
        }
        else { line[li++] = *p++; }
    }
    line[li]=0; DrawText(cx, ly, line, 0x000000);
}

Window wins[6]; Window* focused = nullptr; bool show_launchpad = false; bool show_system_menu = false;
Window* drag_win = nullptr; bool resizing=false; int drag_off_x, drag_off_y;
int win_order[6] = {0, 1, 2, 3, 4, 5};
float dock_offset = 80;

int mx=400, my=300; uint8_t m_cyc=0; int8_t m_pkt[3]; bool m_left=false, m_prev_left=false;
bool shift_pressed = false; bool caps_lock = false;

char kbd_map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

char kbd_map_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};
bool kbd_e0 = false; bool needs_redraw = true;

void MouseInit() {
    while(Port::inb(0x64)&1) Port::inb(0x60);
    Port::WaitWrite(); Port::outb(0x64, 0xA8);
    Port::WaitWrite(); Port::outb(0x64, 0x20); Port::WaitRead(); uint8_t status = Port::inb(0x60) | 2;
    Port::WaitWrite(); Port::outb(0x64, 0x60); Port::WaitWrite(); Port::outb(0x60, status);
    Port::WaitWrite(); Port::outb(0x64, 0xD4); Port::outb(0x60, 0xFF); Port::WaitRead(); Port::inb(0x60); Port::WaitRead(); Port::inb(0x60); Port::WaitRead(); Port::inb(0x60);
    Port::WaitWrite(); Port::outb(0x64, 0xD4); Port::outb(0x60, 0xF4); Port::WaitRead(); Port::inb(0x60);
}

void MouseUpdate() {
   while(Port::inb(0x64) & 1) {
       uint8_t status = Port::inb(0x64);
       if(status & 0x20) { 
           uint8_t b = Port::inb(0x60);
           if(m_cyc==0) { if((b&8)==8){ m_pkt[0]=b; m_cyc++; } }
           else if(m_cyc==1) { m_pkt[1]=b; m_cyc++; }
           else {
               m_pkt[2]=b; m_cyc=0; m_prev_left=m_left; m_left=(m_pkt[0]&1);
               int dx=m_pkt[1]; if(m_pkt[0]&0x10) dx|=0xFFFFFF00; int dy=m_pkt[2]; if(m_pkt[0]&0x20) dy|=0xFFFFFF00;
               mx+=dx; my-=dy; if(mx<0) mx=0; if(mx>=Screen::w) mx=Screen::w-1; if(my<0) my=0; if(my>=Screen::h) my=Screen::h-1;
           }
       } else {
           break;
       }
   }
}

void ExecCmd(char* cmd) {
    int len = Utils::strlen(wins[0].buffer);
    auto append = [&](const char* txt) { Utils::strcpy(wins[0].buffer + len, txt); len = Utils::strlen(wins[0].buffer); };

    if(Utils::strncmp(cmd, "ls", 2) == 0) {
        append("\n"); for(int i=0; i<16; i++) { if(fs_files[i].used) { append(fs_files[i].name); append("  "); } }
    } else if(Utils::strncmp(cmd, "clear", 5) == 0) {
         Utils::strcpy(wins[0].buffer, "TiwutOS v3.0 (Rust)\n"); len = Utils::strlen(wins[0].buffer);
    } else if(Utils::strncmp(cmd, "touch ", 6) == 0) { FS_Create(cmd+6); } else if(Utils::strncmp(cmd, "rm ", 3) == 0) { FS_Remove(cmd+3);
    } else if(Utils::strncmp(cmd, "cat ", 4) == 0) { int idx = FS_Find(cmd+4); if(idx != -1) { append("\n"); append(fs_files[idx].content); } else append("\ncat: No such file");
    } else if(Utils::strncmp(cmd, "echo ", 5) == 0) { append("\n"); append(cmd+5);
    } else if(Utils::strncmp(cmd, "whoami", 6) == 0) { append("\nroot");
    } else if(Utils::strncmp(cmd, "uname", 5) == 0) { append("\nTiwutOS v3.0");
    } else if(Utils::strncmp(cmd, "ping ", 5) == 0) {
        int ip[4] = {0,0,0,0}; int p=0;
        for(int i=5; cmd[i]; i++) { if(cmd[i]=='.') p++; else if(cmd[i]>='0' && cmd[i]<='9' && p<4) ip[p] = ip[p]*10 + (cmd[i]-'0'); }
        RTL8139::SendPing(ip[0], ip[1], ip[2], ip[3]); append("\nPinging... "); return; 
    } else if(Utils::strncmp(cmd, "neofetch", 8) == 0) {
        append("\n"); append("   TTTTTTTTT    OS: TiwutOS v3.0\n"); append("       T        Kernel: Rust + C++ Hybrid\n");
    } else if(Utils::strncmp(cmd, "cd", 2) == 0) { append("\n"); } else if(Utils::strncmp(cmd, "pwd", 3) == 0) { append("\n/root"); }
    else if(Utils::strncmp(cmd, "mkdir", 5) == 0) { append("\nNot implemented"); } else if(Utils::strncmp(cmd, "rmdir", 5) == 0) { append("\nNot implemented"); }
    else if(Utils::strncmp(cmd, "cp", 2) == 0) { append("\nNot implemented"); } else if(Utils::strncmp(cmd, "mv", 2) == 0) { append("\nNot implemented"); }
    else if(Utils::strncmp(cmd, "date", 4) == 0) { append("\nSat May 2 18:50:00 2026"); } else if(Utils::strncmp(cmd, "uptime", 6) == 0) { append("\nup 0 minutes"); }
    else if(Utils::strncmp(cmd, "free", 4) == 0) { append("\n             total       used       free\nMem:        128000      16000     112000"); }
    else if(Utils::strncmp(cmd, "top", 3) == 0) { append("\nPID USER PR NI VIRT RES SHR S %CPU %MEM TIME+ COMMAND\n  1 root 20  0    0   0   0 S  0.0  0.0  0:00 init"); }
    else if(Utils::strncmp(cmd, "ps", 2) == 0) { append("\n  PID TTY          TIME CMD\n    1 tty1     00:00:00 init"); }
    else if(Utils::strncmp(cmd, "kill", 4) == 0) { append("\nkill: usage: kill [-s sigspec | -n signum | -sigspec] pid"); }
    else if(Utils::strncmp(cmd, "chmod", 5) == 0) { append("\nchmod: missing operand"); } else if(Utils::strncmp(cmd, "chown", 5) == 0) { append("\nchown: missing operand"); }
    else if(Utils::strncmp(cmd, "grep", 4) == 0) { append("\nUsage: grep [OPTION]... PATTERNS [FILE]..."); } else if(Utils::strncmp(cmd, "find", 4) == 0) { append("\n."); }
    else if(Utils::strncmp(cmd, "wc", 2) == 0) { append("\nwc: missing operand"); } else if(Utils::strncmp(cmd, "tail", 4) == 0) { append("\ntail: missing operand"); }
    else if(Utils::strncmp(cmd, "head", 4) == 0) { append("\nhead: missing operand"); } else if(Utils::strncmp(cmd, "less", 4) == 0) { append("\nMissing filename"); }
    else if(Utils::strncmp(cmd, "man", 3) == 0) { append("\nWhat manual page do you want?"); } else if(Utils::strncmp(cmd, "history", 7) == 0) { append("\n1 history"); }
    else if(Utils::strncmp(cmd, "alias", 5) == 0) { append("\n"); } else if(Utils::strncmp(cmd, "unalias", 7) == 0) { append("\nunalias: usage: unalias [-a] name [name ...]"); }
    else if(Utils::strncmp(cmd, "export", 6) == 0) { append("\n"); } else if(Utils::strncmp(cmd, "env", 3) == 0) { append("\nUSER=root\nHOME=/root\nSHELL=/bin/bash"); }
    else if(Utils::strncmp(cmd, "set", 3) == 0) { append("\nUSER=root\nHOME=/root\nSHELL=/bin/bash"); } else if(Utils::strncmp(cmd, "unset", 5) == 0) { append("\n"); }
    else if(Utils::strncmp(cmd, "df", 2) == 0) { append("\nFilesystem     1K-blocks  Used Available Use% Mounted on\n/dev/ram0         128000 16000    112000  13% /"); }
    else if(Utils::strncmp(cmd, "du", 2) == 0) { append("\n16      ."); } else if(Utils::strncmp(cmd, "fdisk", 5) == 0) { append("\nUsage: fdisk [options] <disk>"); }
    else if(Utils::strncmp(cmd, "mount", 5) == 0) { append("\n/dev/ram0 on / type ramfs (rw)"); } else if(Utils::strncmp(cmd, "umount", 6) == 0) { append("\numount: missing operand"); }
    else if(Utils::strncmp(cmd, "tar", 3) == 0) { append("\ntar: You must specify one of the '-Acdtrux', '--delete' or '--test-label' options"); }
    else if(Utils::strncmp(cmd, "gzip", 4) == 0) { append("\ngzip: compressed data not written to a terminal."); } else if(Utils::strncmp(cmd, "gunzip", 6) == 0) { append("\ngunzip: compressed data not written to a terminal."); }
    else if(Utils::strncmp(cmd, "zip", 3) == 0) { append("\nCopyright (c) 1990-2008 Info-ZIP"); } else if(Utils::strncmp(cmd, "unzip", 5) == 0) { append("\nUnZip 6.00 of 20 April 2009"); }
    else if(Utils::strncmp(cmd, "curl", 4) == 0) { append("\ncurl: try 'curl --help' for more information"); } else if(Utils::strncmp(cmd, "wget", 4) == 0) { append("\nwget: missing URL"); }
    else if(Utils::strncmp(cmd, "ssh", 3) == 0) { append("\nusage: ssh [-46AaCfGgKkMNnqsTtVvXxYy] ..."); } else if(Utils::strncmp(cmd, "ftp", 3) == 0) { append("\nftp> "); }
    else if(Utils::strncmp(cmd, "telnet", 6) == 0) { append("\ntelnet> "); } else if(Utils::strncmp(cmd, "netstat", 7) == 0) { append("\nActive Internet connections"); }
    else if(Utils::strncmp(cmd, "ifconfig", 8) == 0) { append("\neth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n        inet 10.0.2.15  netmask 255.255.255.0  broadcast 10.0.2.255"); }
    else if(Utils::strncmp(cmd, "ip", 2) == 0) { append("\nUsage: ip [ OPTIONS ] OBJECT { COMMAND | help }"); } else if(Utils::strncmp(cmd, "route", 5) == 0) { append("\nKernel IP routing table\nDestination     Gateway         Genmask         Flags Metric Ref    Use Iface\ndefault         10.0.2.2        0.0.0.0         UG    0      0        0 eth0"); }
    else if(Utils::strncmp(cmd, "arp", 3) == 0) { append("\nAddress                  HWtype  HWaddress           Flags Mask            Iface\n10.0.2.2                 ether   52:54:00:12:35:02   C                     eth0"); }
    else if(Utils::strncmp(cmd, "traceroute", 10) == 0) { append("\nUsage: traceroute [OPTION...] HOST"); } else if(Utils::strncmp(cmd, "nslookup", 8) == 0) { append("\nUsage: nslookup [-opt ...] host"); }
    else if(Utils::strncmp(cmd, "dig", 3) == 0) { append("\nUsage:  dig [@global-server] [domain] [q-type]"); } else if(Utils::strncmp(cmd, "host", 4) == 0) { append("\nUsage: host [-aCdlriTwv] [-c class] [-N ndots] [-t type] [-W time]"); }
    else if(Utils::strncmp(cmd, "ping6", 5) == 0) { append("\nUsage: ping6 [OPTION...] HOST"); }
    else if(Utils::strncmp(cmd, "reboot", 6) == 0) { append("\nRebooting..."); Port::outb(0x64, 0xFE); }
    else if(Utils::strncmp(cmd, "halt", 4) == 0) { append("\nSystem halted."); while(1) asm volatile("hlt"); }
    else if(Utils::strncmp(cmd, "shutdown", 8) == 0) { append("\nShutting down..."); Port::out16(0x604, 0x2000); }
    else if(Utils::strlen(cmd) > 0) { append("\nUnknown: "); append(cmd); }
    
    if(Utils::strncmp(cmd, "clear", 5) != 0) append("\nroot@tiwut:~# "); else append("root@tiwut:~# ");
}

void KeyboardUpdate() {
    uint8_t status = Port::inb(0x64);
    if((status & 1) && !(status & 0x20)) {
        uint8_t sc=Port::inb(0x60); needs_redraw = true;
        if(sc==0xE0) { kbd_e0=true; return; }
        if(kbd_e0) { kbd_e0=false; if(sc==0x5B) show_launchpad=!show_launchpad; return; }
        if(sc == 0x2A || sc == 0x36) shift_pressed = true; else if(sc == 0xAA || sc == 0xB6) shift_pressed = false; else if(sc == 0x3A) caps_lock = !caps_lock;
        if(!(sc&0x80)) {
            char c = shift_pressed ? kbd_map_shift[sc] : kbd_map[sc];
            if(caps_lock && c >= 'a' && c <= 'z') c -= 32; else if(caps_lock && c >= 'A' && c <= 'Z' && !shift_pressed) c += 32;
            if(c && !show_launchpad && focused && focused->active) {
                if(focused->type == APP_TERM) {
                   int len = Utils::strlen(focused->buffer); int prompt_idx = 0;
                   for(int k=0; k<len; k++) if(focused->buffer[k]=='#'&&focused->buffer[k+1]==' ') prompt_idx = k+2;
                   if(c=='\b') { if(len > prompt_idx) focused->buffer[len-1]=0; }
                   else if(c=='\n') { char cmd[64]; int ci=0; for(int k=prompt_idx; k<len; k++) cmd[ci++] = focused->buffer[k]; cmd[ci]=0; ExecCmd(cmd); }
                   else { if(len<2000) { focused->buffer[len]=c; focused->buffer[len+1]=0; } }
                } else if(focused->type == APP_SETTINGS) {
                    if(c=='w' && focused->cursor > 0) focused->cursor -= 20;
                    if(c=='s' && focused->cursor < 2000) focused->cursor += 20;
                } else if(focused->type == APP_EDITOR) {
                    if(c=='\b') { if(focused->cursor>0) focused->buffer[--focused->cursor]=0; } else if(focused->cursor<1000) { focused->buffer[focused->cursor++]=c; focused->buffer[focused->cursor]=0; }
                } else if(focused->type == APP_BROWSER) {
                    if(c=='\b') { if(focused->cursor>0) focused->buffer[--focused->cursor]=0; }
                    else if(c=='\n') { Utils::strcpy(browser_content, "Querying DNS natively...\n"); RTL8139::SendDNS(focused->buffer); }
                    else if(focused->cursor<120) { focused->buffer[focused->cursor++]=c; focused->buffer[focused->cursor]=0; }
                }
            }
        }
    }
}

void OS_Init() { 
    MouseInit(); FS_Init(); Serial::Init();
    wins[0].active=false; wins[0].minimized=false; wins[0].type=APP_TERM; Utils::strcpy(wins[0].title,"Terminal"); wins[0].x=100; wins[0].y=100; wins[0].w=450; wins[0].h=300; Utils::strcpy(wins[0].buffer,"TiwutOS v3.0 (Rust Edition)\nroot@tiwut:~# ");
    wins[1].active=false; wins[1].minimized=false; wins[1].type=APP_EDITOR; Utils::strcpy(wins[1].title,"Notes"); wins[1].x=150; wins[1].y=150; wins[1].w=400; wins[1].h=300; wins[1].cursor=0; wins[1].buffer[0]=0;
    wins[2].active=false; wins[2].minimized=false; wins[2].type=APP_SETTINGS; Utils::strcpy(wins[2].title,"Settings"); wins[2].x=200; wins[2].y=200; wins[2].w=400; wins[2].h=350;
    wins[3].active=false; wins[3].minimized=false; wins[3].type=APP_BROWSER; Utils::strcpy(wins[3].title,"Web Browser"); wins[3].x=250; wins[3].y=100; wins[3].w=500; wins[3].h=350;
    wins[4].active=false; wins[4].minimized=false; wins[4].type=APP_TASKMGR; Utils::strcpy(wins[4].title,"Task Manager"); wins[4].x=300; wins[4].y=150; wins[4].w=400; wins[4].h=250;
    wins[5].active=false; wins[5].minimized=false; wins[5].type=APP_FILEMGR; Utils::strcpy(wins[5].title,"Files"); wins[5].x=350; wins[5].y=200; wins[5].w=400; wins[5].h=300;
    RTL8139::Init();
}

void DrawWindow(Window* w) {
    if(!w->active || w->minimized) return;
    int dw = w->w; int dh = w->h;
    if(w->anim < 1.0f) { dw = (w->w * w->anim); dh = (w->h * w->anim); if(dw<50) dw=50; if(dh<30) dh=30; }
    
    if(Screen::ui_blur) Screen::BlurRegion(w->x, w->y, dw, dh);
    Screen::RoundedRectAlpha(w->x+5, w->y+5, dw, dh, Screen::ui_corner_radius, 0x000000, 80, false, 0);
    uint32_t bg_color = Screen::dark_mode ? 0x222222 : 0xFFFFFF; uint32_t title_color = Screen::dark_mode ? 0x333333 : 0xEBEBEB; uint32_t text_color = Screen::dark_mode ? 0xFFFFFF : 0x000000;
    uint32_t border_col = Screen::dark_mode ? 0x555555 : 0xAAAAAA;
    Screen::RoundedRectAlpha(w->x, w->y, dw, dh, Screen::ui_corner_radius, bg_color, 210, true, border_col);
    Screen::RoundedRectAlpha(w->x, w->y, dw, 30, Screen::ui_corner_radius, title_color, 240, true, border_col);
    if(Screen::ui_corner_radius > 0) Screen::Rect(w->x, w->y+20, dw, 10, title_color);
    
    Screen::RoundedRectAlpha(w->x+10, w->y+8, 12, 12, 6, 0xFF5F56, 255); Screen::RoundedRectAlpha(w->x+30, w->y+8, 12, 12, 6, 0xFFBD2E, 255); Screen::RoundedRectAlpha(w->x+50, w->y+8, 12, 12, 6, 0x27C93F, 255);
    Screen::Rect(w->x+dw-12, w->y+dh-12, 10, 10, 0x888888);
    int tx = w->x + (dw/2) - (Utils::strlen(w->title)*4); DrawText(tx, w->y+8, w->title, Screen::dark_mode ? 0xDDDDDD : 0x555555);
    if(w->anim < 1.0f) return;
    
    int cx = w->x+10; int cy = w->y+40;
    if(w->type==APP_TERM) { 
        Screen::RoundedRectAlpha(w->x+5, w->y+35, w->w-10, w->h-40, 5, 0x111111, 240); 
        int ly=cy; char* p = w->buffer; char line[128]; int li=0;
        while(*p) { if(*p=='\n' || li>=(w->w/8 - 4)) { line[li]=0; DrawText(cx, ly, line, 0x00FF00); ly+=12; li=0; if(*p=='\n') p++; } else { line[li++] = *p++; } }
        line[li]=0; DrawText(cx, ly, line, 0x00FF00); DrawText(cx+(li*8), ly, "_", 0x00FF00);
    } else if(w->type==APP_EDITOR) { DrawText(cx, cy, w->buffer, text_color); DrawText(cx+(Utils::strlen(w->buffer)*8), cy, "|", text_color);
    } else if(w->type==APP_SETTINGS) {
        Screen::RoundedRectAlpha(cx, cy, 120, dh-50, 4, Screen::dark_mode ? 0x333333 : 0xDDDDDD, 255);
        DrawText(cx+10, cy+10, "Display", w->cursor==0 ? 0x00AEEF : text_color);
        DrawText(cx+10, cy+40, "Theme & UI", w->cursor==1 ? 0x00AEEF : text_color);
        DrawText(cx+10, cy+70, "System", w->cursor==2 ? 0x00AEEF : text_color);
        
        int px = cx + 130;
        if(w->cursor == 0) {
            DrawText(px, cy+10, "Screen Resolution", text_color);
            Screen::RoundedRectAlpha(px, cy+30, 80, 25, 4, 0x555555, 255); DrawText(px+10, cy+38, "WQHD", 0xFFFFFF);
            Screen::RoundedRectAlpha(px+90, cy+30, 80, 25, 4, 0x555555, 255); DrawText(px+100, cy+38, "HD", 0xFFFFFF);
            Screen::RoundedRectAlpha(px+180, cy+30, 80, 25, 4, 0x555555, 255); DrawText(px+190, cy+38, "720p", 0xFFFFFF);
        } else if(w->cursor == 1) {
            DrawText(px, cy+10, "Appearance", text_color);
            Screen::RoundedRectAlpha(px, cy+30, 150, 25, 4, Screen::dark_mode ? 0x00AEEF : 0x555555, 255); DrawText(px+10, cy+38, "Dark Mode Toggle", 0xFFFFFF);
            Screen::RoundedRectAlpha(px, cy+65, 150, 25, 4, Screen::ui_blur ? 0x00AEEF : 0x555555, 255); DrawText(px+10, cy+73, "Glass Blur", 0xFFFFFF);
            Screen::RoundedRectAlpha(px, cy+100, 150, 25, 4, Screen::ui_animations ? 0x00AEEF : 0x555555, 255); DrawText(px+10, cy+108, "Animations", 0xFFFFFF);
            Screen::RoundedRectAlpha(px+160, cy+30, 150, 25, 4, Screen::ui_borders ? 0x00AEEF : 0x555555, 255); DrawText(px+170, cy+38, "1px Borders", 0xFFFFFF);
            Screen::RoundedRectAlpha(px+160, cy+65, 150, 25, 4, Screen::ui_corner_radius>0 ? 0x00AEEF : 0x555555, 255); DrawText(px+170, cy+73, "Rounded Corners", 0xFFFFFF);
        } else if(w->cursor == 2) {
            DrawText(px, cy+10, "System Control", text_color);
            Screen::RoundedRectAlpha(px, cy+30, 120, 25, 4, 0xAA2222, 255); DrawText(px+10, cy+38, "Restart", 0xFFFFFF);
            Screen::RoundedRectAlpha(px+130, cy+30, 120, 25, 4, 0xAA2222, 255); DrawText(px+140, cy+38, "Shutdown", 0xFFFFFF);
        }
    } else if(w->type==APP_BROWSER) { DrawBrowser(w, cx, cy); 
    } else if(w->type==APP_TASKMGR) {
        Screen::RoundedRectAlpha(cx, cy, w->w-20, 20, 4, Screen::dark_mode ? 0x444444 : 0xEEEEEE, 255); DrawText(cx+5, cy+5, "PID   PROCESS         RAM    STATUS    ACT", text_color);
        int ly = cy + 25; const char* names[] = {"Terminal", "Notes", "Settings", "Browser", "TaskMgr", "FileMgr"};
        for(int i=0; i<6; i++) {
            char line[64]; Utils::strcpy(line, "00"); Utils::itoa(i+1, line+2); Utils::strcpy(line+4, "  "); Utils::strcpy(line+6, names[i]);
            int len = Utils::strlen(line); while(len < 22) line[len++] = ' '; line[len] = 0; Utils::strcpy(line+22, "1.2 MB ");
            if(wins[i].active) Utils::strcpy(line+29, " RUNNING"); else Utils::strcpy(line+29, " SLEEPING");
            DrawText(cx+5, ly, line, text_color);
            if(wins[i].active) { Screen::RoundedRectAlpha(cx+w->w-80, ly-2, 50, 16, 4, 0xFF3333, 255); DrawText(cx+w->w-70, ly+2, "KILL", 0xFFFFFF); }
            ly += 20;
        }
        Screen::Rect(cx, ly+10, w->w-20, 1, 0x888888); DrawText(cx+5, ly+20, "CPU: 2%   MEM: 16MB / 128MB   TASKS: 6", text_color);
    } else if(w->type==APP_FILEMGR) {
        Screen::RoundedRectAlpha(cx, cy, w->w-20, 30, 4, Screen::dark_mode ? 0x444444 : 0xEEEEEE, 255);
        Screen::RoundedRectAlpha(cx+5, cy+5, 80, 20, 4, 0x00AEEF, 255); DrawText(cx+10, cy+10, "New File", 0xFFFFFF);
        Screen::RoundedRectAlpha(cx+95, cy+5, 90, 20, 4, 0x00AEEF, 255); DrawText(cx+100, cy+10, "New Folder", 0xFFFFFF);
        Screen::RoundedRectAlpha(cx+195, cy+5, 70, 20, 4, 0xFF3333, 255); DrawText(cx+200, cy+10, "Delete", 0xFFFFFF);
        Screen::RoundedRectAlpha(cx+275, cy+5, 70, 20, 4, 0x33CC33, 255); DrawText(cx+280, cy+10, "Format", 0xFFFFFF);
        
        Screen::Rect(cx, cy+40, w->w-20, 1, 0x888888);
        DrawText(cx+5, cy+50, "NAME            TYPE    SIZE   ACTIONS", text_color);
        int ly = cy + 70;
        for(int i=0; i<16; i++) {
            if(fs_files[i].used) {
                DrawIcon(cx+5, ly-10, 2); 
                DrawText(cx+45, ly, fs_files[i].name, text_color);
                DrawText(cx+160, ly, "TXT", text_color);
                DrawText(cx+220, ly, "1KB", text_color);
                Screen::RoundedRectAlpha(cx+280, ly-5, 50, 15, 4, 0x222222, 255); DrawText(cx+285, ly-2, "OPEN", 0xFFFFFF);
                Screen::RoundedRectAlpha(cx+340, ly-5, 50, 15, 4, 0xAA2222, 255); DrawText(cx+345, ly-2, "DEL", 0xFFFFFF);
                ly += 40;
            }
        }
    }
}

void OS_Draw() {
    Screen::DrawBackground(); 
    Screen::BlurRegion(0, 0, Screen::w, 28);
    Screen::RectAlpha(0, 0, Screen::w, 28, 0xFFFFFF, 150);
    
    if(Screen::logo_image) Screen::DrawLogoImage(4, 2, 24, 24);
    else DrawText(10, 8, "Menu", 0);
    DrawText(Screen::w-80, 8, "22:07", 0);
    
    for(int i=0; i<6; i++) DrawWindow(&wins[win_order[i]]);
    
    int dX=(Screen::w-440)/2; int dY=Screen::h-80 + dock_offset; 
    Screen::BlurRegion(dX, dY, 440, 64);
    Screen::RoundedRectAlpha(dX, dY, 440, 64, 16, Screen::dark_mode ? 0x222222 : 0xFFFFFF, 120, true, 0x555555);
    
    DrawIcon(dX+20, dY+16, 1); DrawIcon(dX+90, dY+16, 2); DrawIcon(dX+160, dY+16, 3); DrawIcon(dX+230, dY+16, 4); DrawIcon(dX+300, dY+16, 5); DrawIcon(dX+370, dY+16, 2);
    
    for(int i=0; i<6; i++) {
        if(wins[i].active && !wins[i].minimized) Screen::RoundedRectAlpha(dX+32 + (i*70), dY+54, 8, 4, 2, Screen::dark_mode ? 0xFFFFFF : 0x000000, 255);
        else if(wins[i].active && wins[i].minimized) Screen::RoundedRectAlpha(dX+34 + (i*70), dY+54, 4, 4, 2, Screen::dark_mode ? 0xAAAAAA : 0x555555, 255);
    }
    
    if(show_launchpad) { 
        Screen::BlurRegion(0, 0, Screen::w, Screen::h);
        Screen::RectAlpha(0, 0, Screen::w, Screen::h, 0x000000, 120); 
        int cx=Screen::w/2-250; int cy=Screen::h/2-100; 
        DrawIcon(cx,cy,1); DrawText(cx,cy+40,"Terminal",0xFFFFFF); 
        DrawIcon(cx+100,cy,2); DrawText(cx+100,cy+40,"Notes",0xFFFFFF); 
        DrawIcon(cx+200,cy,3); DrawText(cx+200,cy+40,"Settings",0xFFFFFF); 
        DrawIcon(cx+300,cy,4); DrawText(cx+300,cy+40,"Browser",0xFFFFFF); 
        DrawIcon(cx+400,cy,5); DrawText(cx+400,cy+40,"TaskMgr",0xFFFFFF);
        DrawIcon(cx+200,cy+100,2); DrawText(cx+200,cy+140,"Files",0xFFFFFF); 
    }
    
    if(show_system_menu) {
        Screen::BlurRegion(4, 28, 150, 90);
        Screen::RoundedRectAlpha(4, 28, 150, 90, 8, Screen::dark_mode ? 0x222222 : 0xFFFFFF, 210, true, 0x555555);
        uint32_t tc = Screen::dark_mode ? 0xFFFFFF : 0x000000;
        DrawText(20, 44, "Shut Down", tc);
        DrawText(20, 64, "Restart", tc);
        DrawText(20, 84, "Sleep", tc);
    }
    
    Screen::Swap();
}

void BringToFront(int idx) { int pos = -1; for(int i=0; i<6; i++) if(win_order[i] == idx) pos=i; for(int i=pos; i<5; i++) win_order[i] = win_order[i+1]; win_order[5] = idx; }

void OS_Update() {
   MouseUpdate(); KeyboardUpdate(); if(RTL8139::found) RTL8139::PollReceive();
   while(Serial::HasReceived()) {
       char c = Serial::Read(); int l = Utils::strlen(browser_content);
       if(Utils::strcmp(browser_content, "Loading...\n") == 0) { browser_content[0] = 0; l = 0; }
       if(l < 4000 && (c == '\n' || (c >= 32 && c <= 126))) { browser_content[l] = c; browser_content[l+1] = 0; needs_redraw = true; }
   }
   if(m_left) {
       if(!drag_win && !m_prev_left) {
           if(show_system_menu) {
               if(mx>4 && mx<154 && my>28 && my<120) {
                   if(my>38 && my<58) { Port::out16(0x604, 0x2000); } 
                   else if(my>58 && my<78) { Port::outb(0x64, 0xFE); } 
                   else if(my>78 && my<98) { while(1) asm("hlt"); } 
               }
               show_system_menu = false; needs_redraw = true; return;
           }
           if(mx>2 && mx<30 && my>2 && my<26) { show_system_menu = true; needs_redraw = true; return; }
           
           if(show_launchpad) { 
               show_launchpad=false; needs_redraw = true; 
               int cx=Screen::w/2-250; int cy=Screen::h/2-100;
               if(mx>cx && mx<cx+32 && my>cy && my<cy+32) { wins[0].active=true; wins[0].minimized=false; wins[0].anim=0; BringToFront(0); focused=&wins[0]; return; }
               if(mx>cx+100 && mx<cx+132 && my>cy && my<cy+32) { wins[1].active=true; wins[1].minimized=false; wins[1].anim=0; BringToFront(1); focused=&wins[1]; return; }
               if(mx>cx+200 && mx<cx+232 && my>cy && my<cy+32) { wins[2].active=true; wins[2].minimized=false; wins[2].anim=0; BringToFront(2); focused=&wins[2]; return; }
               if(mx>cx+300 && mx<cx+332 && my>cy && my<cy+32) { wins[3].active=true; wins[3].minimized=false; wins[3].anim=0; BringToFront(3); focused=&wins[3]; return; }
               if(mx>cx+400 && mx<cx+432 && my>cy && my<cy+32) { wins[4].active=true; wins[4].minimized=false; wins[4].anim=0; BringToFront(4); focused=&wins[4]; return; }
               if(mx>cx+200 && mx<cx+232 && my>cy+100 && my<cy+132) { wins[5].active=true; wins[5].minimized=false; wins[5].anim=0; BringToFront(5); focused=&wins[5]; return; }
           }
           else {
               bool hit = false;
               for(int k=5; k>=0; k--) {
                   int i = win_order[k];
                   if(wins[i].active && !wins[i].minimized) {
                       if(mx >= wins[i].x && mx <= wins[i].x+wins[i].w && my >= wins[i].y && my <= wins[i].y+wins[i].h) {
                           BringToFront(i); focused = &wins[i]; hit = true;
                           if(mx >= wins[i].x+wins[i].w-15 && my >= wins[i].y+wins[i].h-15) { drag_win = &wins[i]; resizing = true; drag_off_x = mx - wins[i].w; drag_off_y = my - wins[i].h; }
                           else if(my <= wins[i].y+30) { drag_win = &wins[i]; resizing = false; drag_off_x = mx - wins[i].x; drag_off_y = my - wins[i].y; }
                           if(my <= wins[i].y+30 && mx <= wins[i].x+25) { wins[i].active = false; wins[i].anim = 0; }
                           else if(my <= wins[i].y+30 && mx >= wins[i].x+25 && mx <= wins[i].x+45) { wins[i].minimized = true; }
                           else if(wins[i].type == APP_SETTINGS) {
                               int cx = wins[i].x + 10; int cy = wins[i].y + 40;
                               if(mx > cx && mx < cx+120) {
                                   if(my > cy+5 && my < cy+30) wins[i].cursor = 0;
                                   if(my > cy+35 && my < cy+60) wins[i].cursor = 1;
                                   if(my > cy+65 && my < cy+90) wins[i].cursor = 2;
                               } else {
                                   int px = cx + 130;
                                   if(wins[i].cursor == 0) {
                                       if(my > cy+30 && my < cy+55) {
                                           if(mx > px && mx < px+80) Screen::SetResolutionBGA(2560, 1440);
                                           if(mx > px+90 && mx < px+170) Screen::SetResolutionBGA(1920, 1080);
                                           if(mx > px+180 && mx < px+260) Screen::SetResolutionBGA(1280, 720);
                                       }
                                   } else if(wins[i].cursor == 1) {
                                       if(my > cy+30 && my < cy+55) {
                                           if(mx > px && mx < px+150) Screen::dark_mode = !Screen::dark_mode;
                                           if(mx > px+160 && mx < px+310) Screen::ui_borders = !Screen::ui_borders;
                                       }
                                       if(my > cy+65 && my < cy+90) {
                                           if(mx > px && mx < px+150) Screen::ui_blur = !Screen::ui_blur;
                                           if(mx > px+160 && mx < px+310) Screen::ui_corner_radius = Screen::ui_corner_radius==0 ? 10 : 0;
                                       }
                                       if(my > cy+100 && my < cy+125) {
                                           if(mx > px && mx < px+150) Screen::ui_animations = !Screen::ui_animations;
                                       }
                                   } else if(wins[i].cursor == 2) {
                                       if(my > cy+30 && my < cy+55) {
                                           if(mx > px && mx < px+120) Port::outb(0x64, 0xFE);
                                           if(mx > px+130 && mx < px+250) Port::out16(0x604, 0x2000);
                                       }
                                   }
                               }
                           }
                           else if(wins[i].type == APP_FILEMGR) {
                               if(my > wins[i].y+40 && my < wins[i].y+70) {
                                   if(mx > wins[i].x+15 && mx < wins[i].x+95) FS_Create("new_file.txt");
                                   if(mx > wins[i].x+105 && mx < wins[i].x+195) FS_Create("new_folder");
                                   if(mx > wins[i].x+205 && mx < wins[i].x+275) FS_Remove("new_file.txt");
                                   if(mx > wins[i].x+285 && mx < wins[i].x+355) FS_Init();
                               }
                               else if(my > wins[i].y+110) {
                                   int row = (my - (wins[i].y+110)) / 40;
                                   int act_y = wins[i].y + 110 + (row * 40);
                                   if(my > act_y-15 && my < act_y+10) {
                                       int cur_row = 0;
                                       for(int f=0; f<16; f++) {
                                           if(fs_files[f].used) {
                                               if(cur_row == row) {
                                                   if(mx > wins[i].x+290 && mx < wins[i].x+340) {
                                                       wins[1].active = true; wins[1].minimized = false; BringToFront(1); focused = &wins[1];
                                                       Utils::strcpy(wins[1].buffer, fs_files[f].content);
                                                   }
                                                   if(mx > wins[i].x+350 && mx < wins[i].x+400) FS_Remove(fs_files[f].name);
                                               }
                                               cur_row++;
                                           }
                                       }
                                   }
                               }
                           }
                           else if(wins[i].type == APP_TASKMGR && mx > wins[i].x+wins[i].w-80 && mx < wins[i].x+wins[i].w-30 && my > wins[i].y+65 && my < wins[i].y+185) {
                               int idx = (my - (wins[i].y+65)) / 20;
                               if(idx >= 0 && idx < 6) { wins[idx].active = false; wins[idx].anim = 0; }
                           }
                           return;
                       }
                   }
               }
               if(!hit) {
                   int dX=(Screen::w-440)/2; int dY=Screen::h-80 + dock_offset;
                   if(my>dY && my<dY+64) {
                       if(mx>dX+20 && mx<dX+52) { wins[0].active=true; wins[0].minimized=!wins[0].minimized; BringToFront(0); focused=&wins[0]; if(wins[0].anim==1) wins[0].anim=0; }
                       else if(mx>dX+90 && mx<dX+122) { wins[1].active=true; wins[1].minimized=!wins[1].minimized; BringToFront(1); focused=&wins[1]; if(wins[1].anim==1) wins[1].anim=0; }
                       else if(mx>dX+160 && mx<dX+192) { wins[2].active=true; wins[2].minimized=!wins[2].minimized; BringToFront(2); focused=&wins[2]; if(wins[2].anim==1) wins[2].anim=0; }
                       else if(mx>dX+230 && mx<dX+262) { wins[3].active=true; wins[3].minimized=!wins[3].minimized; BringToFront(3); focused=&wins[3]; if(wins[3].anim==1) wins[3].anim=0; }
                       else if(mx>dX+300 && mx<dX+332) { wins[4].active=true; wins[4].minimized=!wins[4].minimized; BringToFront(4); focused=&wins[4]; if(wins[4].anim==1) wins[4].anim=0; }
                       else if(mx>dX+370 && mx<dX+402) { wins[5].active=true; wins[5].minimized=!wins[5].minimized; BringToFront(5); focused=&wins[5]; if(wins[5].anim==1) wins[5].anim=0; }
                   }
               }
           }
       }
       else if(drag_win) {
           if(resizing) { drag_win->w = mx - drag_off_x; drag_win->h = my - drag_off_y; if(drag_win->w < 100) drag_win->w = 100; if(drag_win->h < 100) drag_win->h = 100; } 
           else { drag_win->x = mx - drag_off_x; drag_win->y = my - drag_off_y; }
       }
   } else { drag_win = nullptr; resizing = false; }
   
   for(int i=0; i<6; i++) {
       if(wins[i].active && !wins[i].minimized && wins[i].anim < 1.0f) {
           wins[i].anim += 0.05f;
           if(wins[i].anim > 1.0f) wins[i].anim = 1.0f;
           needs_redraw = true;
       }
   }
   
   bool dock_overlap = false;
   if(show_launchpad) dock_overlap = true;
   for(int i=0; i<6; i++) {
       if(wins[i].active && !wins[i].minimized && wins[i].y + wins[i].h > Screen::h - 80 && wins[i].anim > 0.5f) {
           dock_overlap = true;
       }
   }
   if(dock_overlap && my < Screen::h - 80) {
       dock_offset += 10; if(dock_offset > 80) dock_offset = 80;
   } else {
       dock_offset -= 10; if(dock_offset < 0) dock_offset = 0;
   }
}
