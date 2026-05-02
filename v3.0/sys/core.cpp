#include "../api/types.h"

#include "../components/utils.cpp"
#include "../components/screen.cpp"
#include "../drivers/rtl8139.cpp"
#include "../apps/apps.cpp"

extern "C" void cpp_main(multiboot_info* mb) {
     if(mb->fb_addr==0)return;
     if((mb->flags & 8) && mb->mods_c > 0) {
         multiboot_module* mod = (multiboot_module*)mb->mods_a;
         Screen::bg_image = (uint8_t*)mod[0].mod_start;
         if(mb->mods_c > 1) Screen::logo_image = (uint8_t*)mod[1].mod_start;
     }
     Screen::Init(mb); OS_Init();
     
     Screen::DrawBackground();
     for(int delay=0; delay<200; delay++) {
         Screen::Rect(0, 0, Screen::w, Screen::h, 0x111111);
         uint8_t alpha = delay < 50 ? delay*5 : (delay > 150 ? (200-delay)*5 : 255);
         
         if(Screen::logo_image) {
             if(alpha == 255) Screen::DrawLogoImage(Screen::w/2 - 64, Screen::h/2 - 80, 128, 128);
             else {
                 for(int y=0; y<128; y++) {
                     for(int x=0; x<128; x++) {
                         int offset = *(uint32_t*)(Screen::logo_image + 10);
                         int bw = *(int*)(Screen::logo_image + 18);
                         int bh = *(int*)(Screen::logo_image + 22);
                         int sy = bh - 1 - ((y * bh)/128); int sx = ((x * bw)/128) * 4;
                         uint8_t pb = Screen::logo_image[offset + sy*bw*4 + sx];
                         uint8_t pg = Screen::logo_image[offset + sy*bw*4 + sx+1];
                         uint8_t pr = Screen::logo_image[offset + sy*bw*4 + sx+2];
                         uint8_t pa = Screen::logo_image[offset + sy*bw*4 + sx+3];
                         if(pa > 0) Screen::PutAlpha(Screen::w/2 - 64 + x, Screen::h/2 - 80 + y, (pr<<16)|(pg<<8)|pb, (pa * alpha)/255);
                     }
                 }
             }
         } else { Screen::RoundedRectAlpha(Screen::w/2 - 64, Screen::h/2 - 80, 128, 128, 20, 0x555555, alpha); }
         
         Screen::RoundedRectAlpha(Screen::w/2 - 100, Screen::h/2 + 60, 200, 10, 5, 0x333333, alpha);
         Screen::RoundedRectAlpha(Screen::w/2 - 100, Screen::h/2 + 60, delay, 10, 5, 0x00AEEF, alpha);
         Screen::Swap();
         for(volatile int i=0; i<300000; i++); 
     }
     
     int old_mx = -1, old_my = -1;
     while(true) { 
         OS_Update(); 
         
         if (m_left) needs_redraw = true;
         
         if (needs_redraw) {
             OS_Draw();
             Screen::DrawCursorFront(mx, my);
             old_mx = mx; old_my = my;
             needs_redraw = false;
         } else {
             if (mx != old_mx || my != old_my) {
                 if (old_mx != -1) {
                     Screen::EraseCursorFront(old_mx, old_my);
                 }
                 Screen::DrawCursorFront(mx, my);
                 old_mx = mx; old_my = my;
             }
         }
     }
}
