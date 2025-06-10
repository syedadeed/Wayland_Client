#include "src.h"

void draw(Master* m){
    for (int y = 0; y < m->HEIGHT; ++y) {
      for (int x = 0; x < m->WIDTH; ++x) {
        if ((x + y / 8 * 8) % 16 < 8) {
          m->pixels[y * m->WIDTH + x] = 0xFF666666;
        } else {
          m->pixels[y * m->WIDTH + x] = 0xFFEEEEEE;
        }
      }
    }
}

int main(){
    static Master m = {0};
    init_master(&m);
    while (wl_display_dispatch(m.display)) {
        if(!m.window_exist){
            break;
        }
    }
    destroy_master(&m);
    return 0;
}
