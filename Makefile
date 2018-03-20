TITLE_ID = ACCSWIT02
TARGET = Advanced_Account_Switcher
OBJS   = main.o graphics.o font.o

LIBS = -lSceDisplay_stub -lSceCtrl_stub -lSceAppMgr_stub -lSceRegistryMgr_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3 -std=c99
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-pack-vpk -s sce_sys/param.sfo -b eboot.bin -a sce_sys/icon0.png=sce_sys/icon0.png -a sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png -a sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml $@

eboot.bin: $(TARGET).velf
	vita-make-fself $< eboot.bin

%.velf: %.elf
	vita-elf-create $< $@

%.elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf *.velf *.elf *.vpk $(OBJS)
