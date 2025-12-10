#include "kernel.h"

#define ROOT_DIR_SECTOR 2048 + 32  
#define DATA_START_SECTOR 2048 + 32 + 32 

uint8_t disk_buf[512]; 
uint8_t file_buf[512];

struct DirEnt {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t res[8];
    uint16_t ch; uint16_t t; uint16_t d; uint16_t cl;
    uint32_t size;
} __attribute__((packed));

char current_path[32] = "/";

void parse_filename(char* input, char* name, char* ext) {
    for(int i=0; i<8; i++) name[i] = ' ';
    for(int i=0; i<3; i++) ext[i] = ' ';
    
    int i=0, j=0;
    while(input[i] && input[i]!='.') { 
        if(j<8) {
            char c = input[i];
            if(c>='a' && c<='z') c -= 32;
            name[j++] = c;
        }
        i++; 
    }
    if(input[i] == '.') { 
        i++; j=0; 
        while(input[i]) { 
            if(j<3) {
                char c = input[i];
                if(c>='a' && c<='z') c -= 32;
                ext[j++] = c;
            }
            i++; 
        } 
    }
}

int match_filename(struct DirEnt* entry, char* input) {
    char p_name[8], p_ext[3];
    parse_filename(input, p_name, p_ext);
    return (strncmp((char*)entry->name, p_name, 8)==0 && strncmp((char*)entry->ext, p_ext, 3)==0);
}

int fs_find_file(char* filename, uint32_t* sector_out, uint32_t* size_out) {
    ata_read_sector(ROOT_DIR_SECTOR, disk_buf);
    struct DirEnt* dir = (struct DirEnt*)disk_buf;
    
    for(int i=0; i<16; i++) {
        if(dir[i].name[0]==0) break;
        if(dir[i].name[0]!=0xE5 && dir[i].attr!=0x0F) {
            if(match_filename(&dir[i], filename)) {
                *sector_out = DATA_START_SECTOR + i; 
                *size_out = dir[i].size;
                return i;
            }
        }
    }
    return -1;
}

void cmd_ls() {
    ata_read_sector(ROOT_DIR_SECTOR, disk_buf);
    struct DirEnt* dir = (struct DirEnt*)disk_buf;
    print_c("TYPE     SIZE    NAME\n", YELLOW, BLACK);
    print_c("----     ----    ----\n", DGREY, BLACK);
    for(int i=0; i<16; i++) {
        if(dir[i].name[0]==0) break;
        if(dir[i].name[0]!=0xE5 && dir[i].attr!=0x0F) {
            if(dir[i].attr & 0x10) print("DIR      "); else print("FILE     ");
            print("512B    ");
            
            char t[13]; int k=0;
            for(int j=0;j<8;j++) if(dir[i].name[j]!=' ') t[k++]=dir[i].name[j];
            if(dir[i].ext[0]!=' ') { t[k++]='.'; for(int j=0;j<3;j++) if(dir[i].ext[j]!=' ') t[k++]=dir[i].ext[j]; }
            t[k]=0; print(t); print("\n");
        }
    }
}

void cmd_touch(char* fname) {
    if(fs_find_file(fname, (uint32_t*)file_buf, (uint32_t*)file_buf) != -1) { print("Error: File exists.\n"); return; }
    
    ata_read_sector(ROOT_DIR_SECTOR, disk_buf);
    struct DirEnt* dir = (struct DirEnt*)disk_buf;
    
    for(int i=0; i<16; i++) {
        if(dir[i].name[0]==0 || dir[i].name[0]==0xE5) {
            parse_filename(fname, (char*)dir[i].name, (char*)dir[i].ext);
            dir[i].attr = 0x20;
            dir[i].size = 512;
            ata_write_sector(ROOT_DIR_SECTOR, disk_buf);
            for(int k=0; k<512; k++) file_buf[k]=0;
            ata_write_sector(DATA_START_SECTOR + i, file_buf);
            print("File created.\n");
            return;
        }
    }
    print("Error: Directory full.\n");
}

void cmd_rm(char* fname) {
    ata_read_sector(ROOT_DIR_SECTOR, disk_buf);
    struct DirEnt* dir = (struct DirEnt*)disk_buf;
    
    for(int i=0; i<16; i++) {
        if(match_filename(&dir[i], fname)) {
            dir[i].name[0] = 0xE5;
            ata_write_sector(ROOT_DIR_SECTOR, disk_buf);
            print("Deleted.\n");
            return;
        }
    }
    print("Error: File not found.\n");
}

void cmd_cp(char* src, char* dest) {
    uint32_t sec, size;
    if(fs_find_file(src, &sec, &size) == -1) { print("Error: Source not found.\n"); return; }
    
    ata_read_sector(sec, file_buf);
    
    cmd_touch(dest);
    
    int dest_idx = fs_find_file(dest, &sec, &size);
    if(dest_idx != -1) {
        ata_write_sector(sec, file_buf);
        print("Copied.\n");
    }
}

void cmd_mv(char* src, char* dest) {
    ata_read_sector(ROOT_DIR_SECTOR, disk_buf);
    struct DirEnt* dir = (struct DirEnt*)disk_buf;

    for(int i=0; i<16; i++) {
        if(match_filename(&dir[i], src)) {
            parse_filename(dest, (char*)dir[i].name, (char*)dir[i].ext);
            ata_write_sector(ROOT_DIR_SECTOR, disk_buf);
            print("Renamed.\n");
            return;
        }
    }
    print("Error: File not found.\n");
}

void cmd_cat(char* fname) {
    uint32_t sec, size;
    if(fs_find_file(fname, &sec, &size) == -1) { print("Error: File not found.\n"); return; }
    ata_read_sector(sec, file_buf);
    print("\n--- BEGIN ---\n");
    for(int i=0; i<512 && file_buf[i]; i++) { 
        char t[2]={file_buf[i],0}; print(t); 
    }
    print("\n--- END ---\n");
}

void cmd_grep(char* term, char* fname) {
    uint32_t sec, size;
    if(fs_find_file(fname, &sec, &size) == -1) { print("Error: File not found.\n"); return; }
    ata_read_sector(sec, file_buf);
    
    int len = strlen(term);
    if(len == 0) return;

    for(int i=0; i<512 - len; i++) {
        int match = 1;
        for(int j=0; j<len; j++) {
            if(file_buf[i+j] != term[j]) { match = 0; break; }
        }
        if(match) {
            print("Match found!\nContext: \"");
            int start = (i > 5) ? i-5 : 0;
            for(int k=start; k<i+len+5 && k<512; k++) {
                if(file_buf[k] >= 32) { char t[2]={file_buf[k],0}; print(t); }
            }
            print("\"\n");
            return;
        }
    }
    print("No matches found.\n");
}

void cmd_find(char* partial_name) {
    ata_read_sector(ROOT_DIR_SECTOR, disk_buf);
    struct DirEnt* dir = (struct DirEnt*)disk_buf;
    print("Searching for: "); print(partial_name); print("\n");
    
    int found = 0;
    char term[12];
    int term_len = 0;
    for(int i=0; partial_name[i]; i++) {
        char c = partial_name[i];
        if(c>='a' && c<='z') c -= 32;
        term[term_len++] = c;
    }
    
    for(int i=0; i<16; i++) {
        if(dir[i].name[0]==0) break;
        if(dir[i].name[0]!=0xE5 && dir[i].attr!=0x0F) {
            char fullname[12]; int k=0;
            for(int j=0;j<8;j++) if(dir[i].name[j]!=' ') fullname[k++]=dir[i].name[j];
            if(dir[i].ext[0]!=' ') { fullname[k++]='.'; for(int j=0;j<3;j++) if(dir[i].ext[j]!=' ') fullname[k++]=dir[i].ext[j]; }
            fullname[k] = 0;
            for(int x=0; x < k; x++) {
                int m = 1;
                for(int y=0; y < term_len; y++) {
                    if(fullname[x+y] != term[y]) { m=0; break; }
                }
                if(m) {
                    print("Found: "); print(fullname); print("\n");
                    found = 1;
                    break;
                }
            }
        }
    }
    if(!found) print("No files match pattern.\n");
}

void cmd_nano(char* fname) {
    uint32_t sec, size;
    int idx = fs_find_file(fname, &sec, &size);
    if(idx == -1) { cmd_touch(fname); idx = fs_find_file(fname, &sec, &size); }
    
    ata_read_sector(sec, file_buf);
    clear_screen();
    print_c(" NANO - TAB: Save, ESC: Quit\n", BLACK, LGREY);
    
    int ptr = 0;
    while(file_buf[ptr]) { char t[2]={file_buf[ptr],0}; print(t); ptr++; }

    while(1) {
        while(!(inb(0x64)&1));
        uint8_t s = inb(0x60); if(s&0x80) continue;
        if(s == 0x01) { clear_screen(); break; }
        if(s == 0x0F) { ata_write_sector(sec, file_buf); clear_screen(); print("Saved.\n"); break; }
        
        char map[] = {0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
        char c = map[s];

        if(c=='\b') { 
            if(ptr>0) { 
                ptr--; file_buf[ptr]=0;
                clear_screen(); print_c(" NANO - TAB: Save, ESC: Quit\n", BLACK, LGREY); print((char*)file_buf); 
            } 
        }
        else if(c) { 
            if(ptr<511) { 
                file_buf[ptr++]=c; file_buf[ptr]=0; 
                char t[2]={c,0}; print(t); 
            } 
        }
    }
}

void kernel_main() {
    clear_screen();
    print_c(" TiwutOS v1.2 \n", WHITE, BLUE);
    print("Filesystem mounted (FAT32 Root).\n");

    char buf[128]; 
    int idx=0;

    while(1) {
        print_c("root@tiwut", LGREEN, BLACK);
        print_c(":", WHITE, BLACK);
        print_c(current_path, LBLUE, BLACK);
        print_c("$ ", WHITE, BLACK);

        idx = 0;
        for(int i=0; i<128; i++) buf[i]=0;
        
        while(1) {
            while(!(inb(0x64)&1));
            uint8_t s = inb(0x60); if(s&0x80) continue;
            char map[] = {0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
            char c = map[s];

            if(c == '\n') { print("\n"); break; }
            else if(c == '\b') { if(idx>0) { idx--; set_cursor(cursor_x-1, cursor_y); print(" "); set_cursor(cursor_x-1, cursor_y); } }
            else if(c) { if(idx<127) { buf[idx++]=c; char t[2]={c,0}; print(t); } }
        }

        char cmd[16], arg1[32], arg2[32];
        for(int i=0;i<16;i++) cmd[i]=0;
        for(int i=0;i<32;i++) arg1[i]=0;
        for(int i=0;i<32;i++) arg2[i]=0;

        int i=0, j=0;
        while(buf[i] && buf[i]!=' ') { if(j<15) cmd[j++] = buf[i]; i++; }
        while(buf[i] == ' ') i++;
        
        j=0;
        while(buf[i] && buf[i]!=' ') { if(j<31) arg1[j++] = buf[i]; i++; }
        while(buf[i] == ' ') i++;
        
        j=0;
        while(buf[i] && buf[i]!=' ') { if(j<31) arg2[j++] = buf[i]; i++; }

        if(strcmp(cmd, "ls")==0) cmd_ls();
        else if(strcmp(cmd, "clear")==0) clear_screen();
        else if(strcmp(cmd, "pwd")==0) { print(current_path); print("\n"); }
        else if(strcmp(cmd, "cd")==0) { print("Note: Flat Filesystem (Root only)\n"); }
        else if(strcmp(cmd, "mkdir")==0) { print("Note: Virtual folder (Root only)\n"); }
        else if(strcmp(cmd, "rmdir")==0) { print("Note: Virtual folder (Root only)\n"); }
        else if(strcmp(cmd, "touch")==0) { if(arg1[0]) cmd_touch(arg1); else print("Usage: touch <file>\n"); }
        else if(strcmp(cmd, "rm")==0) { if(arg1[0]) cmd_rm(arg1); else print("Usage: rm <file>\n"); }
        else if(strcmp(cmd, "mv")==0) { if(arg1[0] && arg2[0]) cmd_mv(arg1, arg2); else print("Usage: mv <src> <dest>\n"); }
        else if(strcmp(cmd, "cp")==0) { if(arg1[0] && arg2[0]) cmd_cp(arg1, arg2); else print("Usage: cp <src> <dest>\n"); }
        else if(strcmp(cmd, "cat")==0) { if(arg1[0]) cmd_cat(arg1); else print("Usage: cat <file>\n"); }
        else if(strcmp(cmd, "more")==0) cmd_cat(arg1);
        else if(strcmp(cmd, "less")==0) cmd_cat(arg1);
        else if(strcmp(cmd, "echo")==0) { print(arg1); print(" "); print(arg2); print("\n"); }
        else if(strcmp(cmd, "nano")==0 || strcmp(cmd, "vi")==0) { if(arg1[0]) cmd_nano(arg1); else print("Usage: nano <file>\n"); }
        else if(strcmp(cmd, "find")==0) { if(arg1[0]) cmd_find(arg1); else print("Usage: find <name>\n"); }
        else if(strcmp(cmd, "grep")==0) { if(arg1[0] && arg2[0]) cmd_grep(arg1, arg2); else print("Usage: grep <term> <file>\n"); }
        else if(strcmp(cmd, "date")==0) get_rtc_time();
        else if(strcmp(cmd, "whoami")==0) print("root\n");
        else if(strcmp(cmd, "ps")==0) { print("PID  USER  CMD\n1    root  kernel\n2    root  bash\n"); }
        else if(strcmp(cmd, "kill")==0) print("Error: Permission denied.\n");
        else if(strcmp(cmd, "exit")==0) { print("Halted."); while(1) __asm__("hlt"); }
        else if(strcmp(cmd, "help")==0 || strcmp(cmd, "man")==0) {
            print_c("Available Commands:\n", LGREEN, BLACK);
            print("File: ls, touch, rm, mv, cp, cat, nano, grep, find\n");
            print("Sys:  date, clear, whoami, ps, kill, exit\n");
            print("Nav:  pwd, cd, mkdir (Virtual/Stubbed)\n");
        }
        else if(strlen(cmd)>0) print("Unknown command.\n");
    }
}