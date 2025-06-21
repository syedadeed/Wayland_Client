#include "src.h"

void init_master(Master* m){
    m->WIDTH = 500;
    m->HEIGHT = 400;
    m->PIXEL_SIZE = 4;
    m->PIXEL_FORMAT = WL_SHM_FORMAT_ARGB8888;
    m->pixels = NULL;
    m->window_exist = 1;
    m->window_resized = 0;

    m->registry_listner.global = global_object_handler;
    m->xdg_shell_listener.ping = xdg_ping;
    m->xdg_base_surface_listener.configure = xdg_surface_configure;
    m->frame_cb_listener.done = new_frame;
    m->toplevel_listener.configure = toplevel_configure;
    m->toplevel_listener.close = toplevel_closed;
    m->toplevel_listener.wm_capabilities = toplevel_capabs;

    m->display = wl_display_connect(NULL);
    m->registry = wl_display_get_registry(m->display);
    wl_registry_add_listener(m->registry, &(m->registry_listner), (void*)m);
    wl_display_roundtrip(m->display);

    m->surface = wl_compositor_create_surface(m->compositor);

    m->xdg_base_surface = xdg_wm_base_get_xdg_surface(m->xdg_shell, m->surface);
    xdg_surface_add_listener(m->xdg_base_surface, &(m->xdg_base_surface_listener), (void*)m);
    m->toplevel = xdg_surface_get_toplevel(m->xdg_base_surface);
    xdg_toplevel_add_listener(m->toplevel, &(m->toplevel_listener), (void*)m);

    xdg_toplevel_set_app_id(m->toplevel, "Fuck");
    // xdg_toplevel_set_fullscreen(m->toplevel, m->output_screen);

    wl_surface_commit(m->surface);
}

void global_object_handler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version){
    Master* m = (Master*)data;
    if(!strcmp(interface, wl_compositor_interface.name)){
        m->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }else if(!strcmp(interface, wl_shm_interface.name)){
        m->shared_memory = wl_registry_bind(registry, name, &wl_shm_interface, version);
    }else if(!strcmp(interface, wl_output_interface.name)){
        m->output_screen = wl_registry_bind(registry, name, &wl_output_interface, version);
    }else if(!strcmp(interface, xdg_wm_base_interface.name)){
        m->xdg_shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(m->xdg_shell, &(m->xdg_shell_listener), (void*)m);
    }
}

void toplevel_capabs(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities){
    Master* m = (Master*)data;
}

void toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states){
    Master* m = (Master*)data;
    if(width == 0 || height == 0) return;

    if(m->WIDTH != width || m->HEIGHT != height){
        if(m->window_resized == 0) munmap(m->pixels, m->WIDTH * m->HEIGHT * m->PIXEL_SIZE);
        m->WIDTH = width;
        m->HEIGHT = height;
        m->window_resized = 1;
    }
}

void xdg_surface_configure(void *data, struct xdg_surface *x_surface, uint32_t serial){
    xdg_surface_ack_configure(x_surface, serial);
    Master* m = (Master*)data;
    if(m->pixels == NULL){
        m->fd = allocate_shared_memory(m->WIDTH * m->HEIGHT * m->PIXEL_SIZE);
        m->pixels = mmap(NULL, m->WIDTH * m->HEIGHT * m->PIXEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m->fd, 0);
        m->shared_memory_pool = wl_shm_create_pool(m->shared_memory, m->fd, m->WIDTH * m->HEIGHT * m->PIXEL_SIZE);
        m->shared_buffer = wl_shm_pool_create_buffer(m->shared_memory_pool, 0, m->WIDTH, m->HEIGHT, m->WIDTH * 4, m->PIXEL_FORMAT); // WIDTH * 4 is the stride
        wl_callback_add_listener(wl_surface_frame(m->surface), &(m->frame_cb_listener), (void*)m);
        draw(m);
        wl_surface_attach(m->surface, m->shared_buffer, 0, 0);
        wl_surface_damage_buffer(m->surface, 0, 0, m->WIDTH, m->HEIGHT);
        wl_surface_commit(m->surface);
    }
}

void new_frame(void *data, struct wl_callback *wl_frame_callback, uint32_t callback_data){
    Master* m = (Master*)data;
    wl_callback_destroy(wl_frame_callback);
    wl_callback_add_listener(wl_surface_frame(m->surface), &(m->frame_cb_listener), (void*)m);
    if(m->window_resized == 1){
        wl_buffer_destroy(m->shared_buffer);
        wl_shm_pool_destroy(m->shared_memory_pool);
        close(m->fd);
        m->fd = allocate_shared_memory(m->WIDTH * m->HEIGHT * m->PIXEL_SIZE);
        m->pixels = mmap(NULL, m->WIDTH * m->HEIGHT * m->PIXEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m->fd, 0);
        m->shared_memory_pool = wl_shm_create_pool(m->shared_memory, m->fd, m->WIDTH * m->HEIGHT * m->PIXEL_SIZE);
        m->shared_buffer = wl_shm_pool_create_buffer(m->shared_memory_pool, 0, m->WIDTH, m->HEIGHT, m->WIDTH * 4, m->PIXEL_FORMAT); // WIDTH * 4 is the stride
        m->window_resized = 0;
    }
    draw(m);
    wl_surface_attach(m->surface, m->shared_buffer, 0, 0);
    wl_surface_damage_buffer(m->surface, 0, 0, m->WIDTH, m->HEIGHT);
    wl_surface_commit(m->surface);
}

void destroy_master(Master* m){
    wl_buffer_destroy(m->shared_buffer);
    close(m->fd);
    wl_display_disconnect(m->display);
}

int32_t allocate_shared_memory(uint64_t size){
    char name[8];
    name[0] = '/';
    name[7] = 0;
    for(int i = 0; i < 6; i++){
        name[i] = (rand() & 23) + 97;
    }
    int32_t file_descriptor = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
    shm_unlink(name);
    ftruncate(file_descriptor, size);
    return file_descriptor;
}

void xdg_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial){
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void toplevel_closed(void *data, struct xdg_toplevel *xdg_toplevel){
    Master* m = (Master*)data;
    m->window_exist = 0;
}
