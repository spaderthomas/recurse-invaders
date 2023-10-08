/*
  You'll probably want to clone the repository to build, because it needs a couple assets in the appropriate path, and the repo obviously has everything set up correctly. I've also vendored the includes for SDL.

  You'll also need SDL2 and SDL_Image installed; I compiled these myself, and included the precompiled libraries in the repo, but if they don't work then you'll need to compile yourself and copy them with the same name to /lib, or to install with your package manager and change the library name in the build script:
  
  g++ -g -std=c++17 \
	-o build/linux/recurse_invaders \
	-Iinclude/sdl2 \
	-Iinclude/sdl_image \
	src/main.cpp \
	lib/sdl-2.28.4-linux-x86_64.a \
	lib/sdl-image-2.6.3-linux-x86_64.a 
 */


#include <algorithm>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_image.h>

typedef uint32_t u32;
typedef int32_t  i32;
typedef float    f32;

struct Vector2 {
	i32 x;
	i32 y;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Surface* surface = nullptr;

constexpr int screen_width = 640; 
constexpr int screen_height = 480;
constexpr int max_path_len = 256;
constexpr f32 frame_time = 1.f / 60.f;

f32 elapsed_time = 0;

struct Player {
	Vector2 position = { 0, 400 };
	Vector2 size = { 64, 64 };
	i32 speed = 4;
};
Player player;

struct Enemy {
	Vector2 position = { 0, 0 };
	Vector2 size = { 64, 64 };
};
Enemy enemy;

namespace invaders::fs {	
    char root [max_path_len] = {0};
    char assets [max_path_len] = {0};

	void set_install_dir(char* buffer, i32 buffer_len) {
		readlink("/proc/self/exe", buffer, buffer_len);
		i32 len = strlen(buffer);

		// We'll be in /build/linux, so we want to strip off both of those
		// directories AND the executable name
		i32 removed = 0;
		for (i32 i = len - 1; i > 0; i--) {
			if (buffer[i] == '/') removed++;
			buffer[i] = 0;
			if (removed == 3) break;
		}
	}

    void init() {
		set_install_dir(root, max_path_len);
		snprintf(assets, max_path_len, "%s/assets", root);
	}

	// Use these to fill in a buffer with a path to something. In other words
	// if you want the path to ship.png, then calling this:
	//   asset("ship.png", buffer, buffer_size
	// would give you this:
	//   "/path/to/repo/assets/ship.png"
    void asset(const char* asset_name, char* buffer, i32 buffer_len) {
		snprintf(buffer, buffer_len, "%s/%s", assets, asset_name);
	}
}


int main(int num_args, char** args) {
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
    }

	if (SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_HIDDEN, &window, &renderer) < 0) {
        SDL_Log("SDL_CreateWindowAndRenderer() failed: %s\n", SDL_GetError());
        return 1;
    }
	SDL_SetWindowTitle(window, "RECURSE INVADERS");
	SDL_SetWindowSize(window, screen_width, screen_height);
	SDL_ShowWindow(window);
	
	surface = SDL_GetWindowSurface(window);

	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);

	// Load assets
	invaders::fs::init();
	
	char ship_path [max_path_len] = {0};
	invaders::fs::asset("ship.png", ship_path, max_path_len);
	auto ship_texture = IMG_LoadTexture(renderer, ship_path);
	if (!ship_texture) {
		SDL_Log("Couldn't load %s: %s\n", ship_path, SDL_GetError());
		return 1;
	}
	i32 ship_width, ship_height = 0;
	SDL_QueryTexture(ship_texture, NULL, NULL, &ship_width, &ship_height);

	char enemy_path [max_path_len] = {0};
	invaders::fs::asset("enemy.png", enemy_path, max_path_len);
	auto enemy_texture = IMG_LoadTexture(renderer, enemy_path);
	if (!enemy_texture) {
		SDL_Log("Couldn't load %s: %s\n", enemy_path, SDL_GetError());
		return 1;
	}
	i32 enemy_width, enemy_height = 0;
	SDL_QueryTexture(enemy_texture, NULL, NULL, &enemy_width, &enemy_height);


	bool move_left = false;
	bool move_right = false;

	SDL_Event event;
	while (true) {
		// Read all input from SDL
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				return 0;
			}
			else if (event.type == SDL_KEYDOWN) {
				auto key_code = event.key.keysym.sym;
				if (key_code == SDLK_LEFT) {
					move_left = true;
				}
				else if (key_code == SDLK_RIGHT) {
					move_right = true;
				}
			}
			else if (event.type == SDL_KEYUP) {
				auto key_code = event.key.keysym.sym;
				if (key_code == SDLK_LEFT) {
					move_left = false;
				}
				else if (key_code == SDLK_RIGHT) {
					move_right = false;
				}

			}
			
		}

		// Run the game update
		if (move_left) player.position.x -= player.speed;
		if (move_right) player.position.x += player.speed;

		auto enemy_max_x = screen_width - enemy.size.x;
		auto oscillator = (sinf(elapsed_time) + 1) / 2;
		enemy.position.x = oscillator * enemy_max_x;

		auto clamp_position = [](Vector2* position, Vector2 size) {
			position->x = std::max(position->x, 0);
			position->x = std::min(position->x, screen_width - size.x);
		};
		clamp_position(&player.position, player.size);
		clamp_position(&enemy.position, enemy.size);

		// Clear the render target, then draw everything, then present the rendered frame
		SDL_RenderClear(renderer);

		{
			SDL_Rect ship_rect = { 0, 0, ship_width, ship_height };
			SDL_Rect draw_rect = { player.position.x, player.position.y, player.size.x, player.size.y };
			SDL_RenderCopy(renderer, ship_texture, &ship_rect, &draw_rect);
		}

		{
			SDL_Rect enemy_rect = { 0, 0, enemy_width, enemy_height };
			SDL_Rect draw_rect = { enemy.position.x, enemy.position.y, enemy.size.x, enemy.size.y };
			SDL_RenderCopy(renderer, enemy_texture, &enemy_rect, &draw_rect);
		}

		SDL_RenderPresent(renderer);

		// Why implement your framerate intelligently when you can just sleep instead...?
		elapsed_time += frame_time;
		SDL_Delay(frame_time * 1000);
	}
}
