g++ -g -std=c++17 \
	-o build/linux/recurse_invaders \
	-Iinclude/sdl2 \
	-Iinclude/sdl_image \
	src/main.cpp \
	lib/sdl-2.28.4-linux-x86_64.a \
	lib/sdl-image-2.6.3-linux-x86_64.a 
	
