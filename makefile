OUT = MAL
CC := clang
TARGET := x86_64-pc-windows-gnu
SDL_TARGET := x86_64-w64-mingw32
SRC = src/dllmain.c src/io.c src/helpers.c src/poll.c tomlc99/toml.c minhook/src/buffer.c minhook/src/hook.c minhook/src/trampoline.c minhook/src/hde/hde32.c minhook/src/hde/hde64.c
OBJ = ${addprefix ${TARGET}/,${SRC:.c=.o}}
CFLAGS = -std=c99 -Iminhook/include -ISDL/${SDL_TARGET}/include -ISDL/include -Itomlc99 -Wall -Ofast -target ${TARGET} -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=_WIN32_WINNT_WIN7 -DSDL_MAIN_HANDLED
LDFLAGS := -shared -static -static-libgcc -s
LIBS := SDL/${SDL_TARGET}/build/.libs/libSDL2.a SDL/${SDL_TARGET}/build/.libs/libSDL2main.a -lmingw32 -luuid -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lsetupapi -lversion
DEPS = SDL

all: options ${OUT}

.PHONY: options
options:
	@mkdir -p ${TARGET}/src
	@mkdir -p ${TARGET}/minhook/src/hde
	@mkdir -p ${TARGET}/tomlc99
	@echo "CFLAGS	= ${CFLAGS}"
	@echo "LDFLAGS	= ${LDFLAGS}"
	@echo "CC	= ${CC}"

.PHONY: dist
dist: options ${OUT}
	git clone https://github.com/gabomdq/SDL_GameControllerDB --recursive
	git clone https://github.com/BroGamer4256/dinput8 --recursive
	mkdir PDL && cd PDL && wget https://github.com/PDModdingCommunity/PD-Loader/releases/download/2.6.5a-r4n/PD-Loader-2.6.5a-r4.zip

	mkdir -p out/plugins/patches/
	mkdir -p "out/plugins/Novidia Shaders"
	cd dinput8 && make TARGET=${TARGET}
	cd PDL && unzip PD-Loader-2.6.5a-r4.zip
	
	cp PDL/plugins/bassasio.dll \
		PDL/plugins/ShaderPatch.ini \
		dist/keyconfig.toml \
		dist/config.toml \
		${TARGET}/MAL.dll \
		SDL_GameControllerDB/gamecontrollerdb.txt \
	out/plugins
	cp dist/patches/example.toml \
		dist/patches/osage.toml \
		dist/patches/good_dwgui_size.toml \
		dist/patches/disable_movies.toml \
	out/plugins/patches
	cp dinput8/${TARGET}/dinput8.dll out/

	cp PDL/plugins/DivaSound.dva out/plugins/DivaSound.dll
	cp PDL/plugins/DivaSound_template.bin out/plugins/DivaSound.ini
	cp PDL/plugins/Novidia.dva out/plugins/Novidia.dll
	cp "PDL/plugins/Novidia Shaders/e6d9187b.vcdiff" "out/plugins/Novidia Shaders/e6d9187b.vcdiff"
	cp PDL/plugins/ShaderPatch.dva out/plugins/ShaderPatch.dll
	cp PDL/plugins/ShaderPatchConfig_template.bin out/plugins/ShaderPatchConfig.ini

	cd out && zip ../dist.zip * plugins/* plugins/patches/* plugins/Novidia\ Shaders/*
	rm -rf PDL SDL_GameControllerDB dinput8

${TARGET}/%.o: %.c
	@echo BUILD $@
	@${CC} -c ${CFLAGS} $< -o $@

.PHONY: ${OUT}
${OUT}: ${DEPS} ${OBJ}
	@echo LINK $@
	@${CC} ${CFLAGS} -o ${TARGET}/$@.dll ${OBJ} ${LDFLAGS} ${LIBS}

.PHONY: fmt
fmt:
	@cd src && clang-format -i *.h *.c -style=file

.PHONY: clean
clean:
	rm -rf ${TARGET}
	rm -rf SDL/${SDL_TARGET}

.PHONY: SDL
SDL:
	@mkdir -p SDL/${SDL_TARGET}
	@cd SDL/${SDL_TARGET} && ../configure --build=x86_64-linux-gnu --host=${SDL_TARGET} --disable-sdl2-config --disable-shared --enable-assertions=release --enable-directx --enable-haptic 
	@make -s -C SDL/${SDL_TARGET}
