You'll also need SDL2 and SDL_Image installed; I compiled these myself, and included the precompiled libraries in the repo, but if they don't work then you'll need to compile yourself and copy them with the same name to /lib, or to install with your package manager and change the library name in the build script.

To build, just run `./build/linux/build.sh` and run with `./build/linux/recurse_invaders`
