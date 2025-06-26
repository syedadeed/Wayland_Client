# gcc main.c src.c xdg-shell-protocol.c -lwayland-client -lxkbcommon && WAYLAND_DEBUG=1 ./a.out && rm a.out
gcc main.c src.c xdg-shell-protocol.c -lwayland-client -lxkbcommon && ./a.out && rm a.out
