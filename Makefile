PHONY := all package clean

DATE := $(shell date +%y-%m-%d)

CC := arm-vita-eabi-gcc
CXX := arm-vita-eabi-g++
STRIP := arm-vita-eabi-strip

PROJECT_TITLE := vOpenTiryan
PROJECT_TITLEID := VOPTIRYAN
PROJECT := vOpenTiryan

LIBS = -lpsp2shell -lpthread -lSDL2 -lvita2d -lSceDisplay_stub -lSceGxm_stub \
		-lSceSysmodule_stub -lSceCtrl_stub -lScePgf_stub -lSceNetCtl_stub \
		-lSceNet_stub -lScePower_stub -lSceKernel_stub -lSceCommonDialog_stub \
		-lSceAppUtil_stub -lSceAudio_stub -lSceAppMgr_stub -lpng -lz -lm -lc


CFLAGS  = -Wl,-q -Wall -O3 -MMD -pedantic -Wall -Wextra \
			-Wno-missing-field-initializers -Wno-unused-parameter \
			-ftree-vectorize -mword-relocations -fomit-frame-pointer -ffast-math \
			-march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard \
			-DVITA -DTARGET_UNIX -std=c99 -I./src -I$(VITASDK)/arm-vita-eabi/include
#-DVDEBUG

OBJS := src/mainint.o src/arg_parse.o src/backgrnd.o src/destruct.o src/fonthand.o \
	src/sizebuf.o src/musmast.o src/loudness.o src/shots.o src/joystick.o src/game_menu.o \
	src/pcxload.o src/pcxmast.o src/lvlmast.o src/player.o src/keyboard.o \
	src/picload.o src/opl.o src/nortvars.o src/episodes.o src/animlib.o src/sndmast.o \
	src/params.o src/scroller.o src/font.o src/vga_palette.o src/sprite.o src/file.o src/lvllib.o \
	src/config.o src/helptext.o src/network.o src/xmas.o src/starlib.o src/opentyr.o src/editship.o \
	src/jukebox.o src/setup.o src/std_support.o src/mouse.o src/nortsong.o \
	src/mtrand.o src/config_file.o src/lds_play.o src/menus.o src/vga256d.o src/tyrian2.o src/palette.o \
	src/varz.o \
	src/video_vita.o src/keyboard_vita.o

all: package

package: $(PROJECT).vpk

$(PROJECT).vpk: eboot.bin param.sfo
	rm -rf vpk && mkdir -p vpk/sce_sys/livearea/contents
	cp eboot.bin vpk/
	cp param.sfo vpk/sce_sys/
	cp vita/icon0.png vpk/sce_sys/
	cp vita/template.xml vpk/sce_sys/livearea/contents/
	cp vita/bg.png vpk/sce_sys/livearea/contents/
	cp vita/startup.png vpk/sce_sys/livearea/contents/
	cp -r data vpk/data
	cd vpk && zip -r ../$(PROJECT)-$(DATE).vpk . && cd ..
	
eboot.bin: $(PROJECT).velf
	vita-make-fself -s $(PROJECT).velf eboot.bin

param.sfo:
	vita-mksfoex -s TITLE_ID="$(PROJECT_TITLEID)" "$(PROJECT_TITLE)" param.sfo

$(PROJECT).velf: $(PROJECT).elf
	$(STRIP) -g $<
	vita-elf-create $< $@

$(PROJECT).elf: $(OBJS)
	$(CC) -Wl,-q -o $@ $^ $(LIBS)

$(OBJ_DIRS):
	mkdir -p $@

out/%.o : src/%.c | $(OBJ_DIRS)
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROJECT).velf $(PROJECT).elf $(PROJECT).vpk param.sfo eboot.bin $(OBJS)
	rm -rf vpk
