#include "xdg-shell-client-protocol.h"
#include <wayland-client.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct{
    int window_exist;
    int WIDTH;
    int HEIGHT;
    int PIXEL_SIZE;
    int PIXEL_FORMAT;
    int32_t fd;
    uint32_t* pixels;
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_registry_listener registry_listner;
    struct wl_output* output_screen;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct wl_callback_listener frame_cb_listener;
    struct wl_shm* shared_memory;
    struct wl_shm_pool* shared_memory_pool;
    struct wl_buffer* shared_buffer;
    struct xdg_wm_base* xdg_shell;
    struct xdg_wm_base_listener xdg_shell_listener;
    struct xdg_surface* xdg_base_surface;
    struct xdg_surface_listener xdg_base_surface_listener;
    struct xdg_toplevel* toplevel;
    struct xdg_toplevel_listener toplevel_listener;
} Master;

void global_object_handler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
void xdg_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
void toplevel_capabs(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities);
void toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
void toplevel_closed(void *data, struct xdg_toplevel *xdg_toplevel);
void new_frame(void *data, struct wl_callback *wl_callback, uint32_t callback_data);
int32_t allocate_shared_memory(uint64_t size);
void draw(Master* m);
void init_master(Master* m);
void destroy_master(Master* m);
