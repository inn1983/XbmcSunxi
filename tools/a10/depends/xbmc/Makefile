include ../depends.mk

CONFIGURE=./configure \
	--prefix=$(PREFIX) \
	--build=$(BUILD) \
	--host=$(HOST) \
	--disable-x11 \
	--disable-sdl \
	--disable-xrandr \
	--disable-joystick \
	--disable-gl \
	--disable-vdpau \
	--disable-vaapi \
	--disable-openmax \
	--enable-neon \
	--enable-gles \
	--disable-mysql \
	--disable-optical-drive \
	--disable-crystalhd \
	--disable-vtbdecoder \
	--disable-pulse \
	--disable-projectm \
	--disable-libcec \
	--disable-optimizations \

SRC=../../../../

all: dep
	(cd $(SRC); make -j$(JOBS))

install:
	(cd $(SRC); make install)

install-only:
	(cd $(SRC); make install)

bin-only:
	(cd $(SRC); make -j$(JOBS))

dep: bootstrap configure

bootstrap:
	(cd $(SRC); ./bootstrap)

configure:
	(cd $(SRC); $(CONFIGURE))