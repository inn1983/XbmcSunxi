include ../depends.mk

MALIGIT=https://github.com/linux-sunxi/sunxi-mali.git
SUNXI-MALI=sunxi-mali
VERSION=r3p0
ABI=armhf
EGL_TYPE=framebuffer

all: .installed

.installed: .git
	(	cd $(SUNXI-MALI) &&\
		make config VERSION=$(VERSION) ABI=$(ABI) EGL_TYPE=$(EGL_TYPE) &&\
		echo "prefix = $(XBMCPREFIX)/" >> Makefile.setup &&\
		rm -rf $(XBMCPREFIX) &&\
		mkdir -p $(XBMCPREFIX)/lib &&\
		mkdir -p $(XBMCPREFIX)/include &&\
		make && make install \
	)
	touch $@

.git:
		git clone $(MALIGIT)
	(	cd $(SUNXI-MALI) &&\
		git submodule init &&\
		git submodule update \
	)
	touch $@

clean:
	rm -rf .installed .git $(SUNXI-MALI)