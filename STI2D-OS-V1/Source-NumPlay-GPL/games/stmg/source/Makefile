# STI2D vs STMG — build NumWorks (.nwa), PC (SDL2) et banc d'essai.
#   make            -> build/sti2d_vs_stmg.nwa  (arm-none-eabi-gcc + nwlink requis)
#   make pc         -> build/sti2d_vs_stmg_pc   (SDL2 requis)
#   make test       -> captures d'écran dans shots/
#   make install    -> installe sur la calculatrice branchée en USB (nwlink)
#   make web        -> build/web/index.html  (version navigateur, clang + wasm-ld requis)

NWLINK ?= npx --yes -- nwlink@1.0.0
CC_ARM ?= arm-none-eabi-gcc
BUILD = build
APP = $(BUILD)/sti2d_vs_stmg.nwa
SRC_GAME = src/game.c src/gfx.c src/maze.c src/sprites.c
SRC_NW = $(SRC_GAME) src/main.c src/platform_nw.c

CFLAGS_COMMON = -std=c99 -Wall -Wextra -Wdouble-promotion -fsingle-precision-constant -fno-math-errno

all: $(APP)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/icon.o: src/icon.png | $(BUILD)
	$(NWLINK) png-icon-o $< $@

NW_CFLAGS = $(shell $(NWLINK) eadk-cflags-device) $(CFLAGS_COMMON) -Os \
  -ffunction-sections -fdata-sections -flto -fno-fat-lto-objects -fwhole-program -fvisibility=internal
NW_LDFLAGS = $(shell $(NWLINK) eadk-ldflags-device) --specs=nano.specs -flinker-output=nolto-rel \
  -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level -Wl,--gc-sections
NW_OBJS = $(patsubst src/%.c,$(BUILD)/nw/%.o,$(SRC_NW))

$(BUILD)/nw/%.o: src/%.c src/*.h
	@mkdir -p $(BUILD)/nw
	$(CC_ARM) $(NW_CFLAGS) -c $< -o $@

$(APP): $(NW_OBJS) $(BUILD)/icon.o
	$(CC_ARM) $(NW_CFLAGS) $(NW_LDFLAGS) $^ -lm -o $@

check: $(APP)
	$(NWLINK) nwa-bin $(APP) $(BUILD)/check.bin && echo "Lien OK"

install: $(APP)
	$(NWLINK) install-nwa $(APP)

pc: $(SRC_GAME) src/main.c src/platform_sdl.c src/*.h | $(BUILD)
	cc $(CFLAGS_COMMON) -O2 $(SRC_GAME) src/main.c src/platform_sdl.c $$(sdl2-config --cflags --libs) -lm -o $(BUILD)/sti2d_vs_stmg_pc

test: $(SRC_GAME) tools/test_shots.c | $(BUILD)
	cc $(CFLAGS_COMMON) -O1 -g -fsanitize=address,undefined -DGAME_DEBUG $(SRC_GAME) tools/test_shots.c -lm -o $(BUILD)/test_shots
	mkdir -p shots && ./$(BUILD)/test_shots shots

WEB_CFLAGS = --target=wasm32 -O2 -std=c99 -ffreestanding -nostdlib -mbulk-memory -DPLATFORM_WEB=1 \
  -Wall -Wextra -Wdouble-promotion -isystem web/libc
WEB_LDFLAGS = -Wl,--no-entry -Wl,--strip-all -Wl,-z,stack-size=65536

web: $(SRC_GAME) web/web_main.c web/libc/* web/shell.html tools/web_build.py src/*.h | $(BUILD)
	mkdir -p $(BUILD)/web
	clang $(WEB_CFLAGS) $(WEB_LDFLAGS) $(SRC_GAME) web/web_main.c web/libc/libc.c -o $(BUILD)/web/game.wasm
	python3 tools/web_build.py $(BUILD)/web/game.wasm web/shell.html $(BUILD)/web

clean:
	rm -rf $(BUILD) shots

.PHONY: all check install pc test web clean
