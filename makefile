OUT = MAL
CC := clang
TARGET := x86_64-pc-windows-gnu
SDL_TARGET := x86_64-w64-mingw32
SRC = src/dllmain.c src/io.c src/helpers.c tomlc99/toml.c minhook/src/buffer.c minhook/src/hook.c minhook/src/trampoline.c minhook/src/hde/hde32.c minhook/src/hde/hde64.c
OBJ = ${addprefix ${TARGET}/,${SRC:.c=.o}}
CFLAGS = -std=c99 -Iminhook/include -ISDL/${SDL_TARGET}/include -ISDL/include -Itomlc99 -Wall -Ofast -target ${TARGET} -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=_WIN32_WINNT_WIN7
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

${TARGET}/%.o: %.c
	@echo BUILD $@
	@${CC} -c ${CFLAGS} $< -o $@

.PHONY: ${OUT}
${OUT}: ${DEPS} ${OBJ}
	@cd src && clang-format -i *.h *.c -style=file
	@echo LINK $@
	@${CC} ${CFLAGS} -o ${TARGET}/$@.dll ${OBJ} ${LDFLAGS} ${LIBS}

.PHONY: clean
clean:
	rm -rf ${TARGET}
	rm -rf SDL/${SDL_TARGET}

.PHONY: SDL
SDL:
	@mkdir -p SDL/${SDL_TARGET}
	@cd SDL/${SDL_TARGET} && ../configure --build=x86_64-linux-gnu --host=${SDL_TARGET} --disable-sdl2-config --disable-shared --enable-assertions=release --enable-directx --enable-haptic 
	@make -s -C SDL/${SDL_TARGET}
