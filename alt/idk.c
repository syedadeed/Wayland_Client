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

int window_exists = 1;
int WIDTH = 200;
int HEIGHT = 100;
int PIXEL_SIZE = 4;
int PIXEL_FORMAT = WL_SHM_FORMAT_ARGB8888;
int32_t fd = -1;

struct wl_display* display;
struct wl_registry* registry;
struct wl_registry_listener registry_listner = {0};
// Alows creation, rendering & composion of surfaces
struct wl_compositor* compositor = NULL;
// A surface is a rectangular area with a local coordinate system
struct wl_surface* surface = NULL;
// Allows creation and management of wl_shm_pools
struct wl_shm* shared_memory = NULL;
// Manages a shared memory region, from which buffers are created
struct wl_shm_pool* shared_memory_pool = NULL;
// A buffer is the memory region that you commit to the wl_srfc, so that it can be displayed
struct wl_buffer* shared_buffer = NULL;
// Pointer to the underlying shared data
uint8_t* pixels = NULL;
//Base global xdg object from which other xdg objects can be created
struct xdg_wm_base* xdg_shell;
struct xdg_wm_base_listener xdg_shell_listener;

int br = 0;

void draw(){
    uint32_t* px = (uint32_t*)pixels;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        if ((x + y / 8 * 8) % 16 < 8) {
          px[y * WIDTH + x] = 0xFF666666;
        } else {
          px[y * WIDTH + x] = 0xFFEEEEEE;
        }
      }
    }
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

void global_object_handler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version){
    if(!strcmp(interface, wl_compositor_interface.name)){
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
        surface = wl_compositor_create_surface(compositor);
    }else if(!strcmp(interface, wl_shm_interface.name)){
        shared_memory = wl_registry_bind(registry, name, &wl_shm_interface, version);
        fd = allocate_shared_memory(WIDTH * HEIGHT * PIXEL_SIZE);
        pixels = mmap(NULL, WIDTH * HEIGHT * PIXEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        shared_memory_pool = wl_shm_create_pool(shared_memory, fd, WIDTH * HEIGHT * PIXEL_SIZE);
        shared_buffer = wl_shm_pool_create_buffer(shared_memory_pool, 0, WIDTH, HEIGHT, WIDTH * 4, PIXEL_FORMAT); // WIDTH * 4 is the stride
    }else if(!strcmp(interface, xdg_wm_base_interface.name)){
        xdg_shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(xdg_shell, &xdg_shell_listener, NULL);
    }
}

int main(){
    //Establishes a connection b/w client & server and returns the connection object
    display = wl_display_connect(NULL);

    // Keeps track of global objects that the compositor offers to the client
    registry = wl_display_get_registry(display);
    registry_listner.global = global_object_handler;
    //The last argument is data, which is sent to the global_object_handler func as "data" arg
    wl_registry_add_listener(registry, &registry_listner, NULL);

    //Blocks further execution until all pending incomming events from the server has been recieved
    wl_display_roundtrip(display);

    while(window_exists){
        //Reads pending events from the wayland socket and calls the appropriate listeners
        wl_display_dispatch(display); //Returns the number of pending events
        if(!br) continue;
        draw();
        wl_surface_attach(surface, shared_buffer, 0, 0);
        wl_surface_damage_buffer(surface, 0, 0, WIDTH, HEIGHT);
        wl_surface_commit(surface);
        wl_display_flush(display); // Sends out all the pending requests
    }

    if(shared_buffer != NULL){
        wl_buffer_destroy(shared_buffer);
    }
    close(fd);
    wl_display_disconnect(display);
    return 0;
}
