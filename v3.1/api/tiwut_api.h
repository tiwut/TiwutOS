#pragma once

#ifndef TIWUT_API_H
#define TIWUT_API_H

#ifdef __cplusplus
extern "C" {
#endif

int syscall_exit(int code);
int syscall_getpid();
void syscall_yield();
int syscall_create_process(const char *app_path);
void syscall_sleep(unsigned int milliseconds);
unsigned long long syscall_get_time();
typedef struct {
  int x, y, w, h;
} Rect;
int window_create(const char *title, int x, int y, int w, int h);
void window_destroy(int win_id);
void window_draw_rect(int win_id, int x, int y, int w, int h,
                      unsigned int color);
void window_draw_text(int win_id, int x, int y, const char *text,
                      unsigned int color);
void window_draw_image(int win_id, int x, int y, const unsigned char *img_data,
                       int w, int h);
void window_swap(int win_id);
void window_set_title(int win_id, const char *title);
void window_set_focus(int win_id);

#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR 0x03
#define O_CREAT 0x04

int fs_open(const char *path, int flags);
int fs_read(int fd, char *buffer, int size);
int fs_write(int fd, const char *buffer, int size);
void fs_close(int fd);
int fs_delete(const char *path);
int fs_mkdir(const char *path);
int fs_get_size(int fd);

#define SOCK_TCP 1
#define SOCK_UDP 2

int net_socket(int type);
int net_connect(int sock, const char *ip, int port);
int net_send(int sock, const char *data, int len);
int net_recv(int sock, char *buffer, int max_len);
void net_close(int sock);
void *hw_allocate_dma(int size);
void hw_free_dma(void *ptr);
int hw_get_screen_width();
int hw_get_screen_height();
void hw_play_sound(int frequency, int duration);
int app_request_permission(const char *perm_name);
int app_check_auth(int otp);

#ifdef __cplusplus
}
#endif

#endif
