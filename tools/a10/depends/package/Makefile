include ../depends.mk

LCLPREFIX=$(shell echo $(XBMCPREFIX) | sed 's/^\///g')

DEB=xbmca10.deb
TARBALL1=xbmca10deb.tar.gz
TARBALL2=xbmca10.tar.gz

all: $(TARBALL1)

$(TARBALL1): root/DEBIAN/control $(XBMCPREFIX)/lib/xbmc/xbmc.bin README
	(	rm -rf tmp &&\
		cp -r root tmp &&\
		cd tmp &&\
		tar cf - $(XBMCPREFIX) | tar xf - &&\
		rm -rf $(LCLPREFIX)/include &&\
		rm -rf $(LCLPREFIX)/lib/pkgconfig &&\
		rm -f  $(LCLPREFIX)/lib/*.a &&\
		$(STRIP) --strip-unneeded `find . -name xbmc.bin -print` &&\
		$(STRIP) --strip-unneeded `find . -name *.so -print` \
	)
	dpkg -b tmp $(DEB)
	tar cvzf $(TARBALL1) README $(DEB)
	rm -r tmp $(DEB)

tarball: $(XBMCPREFIX)/lib/xbmc/xbmc.bin
	(	rm -rf tmp &&\
		mkdir tmp &&\
		cd tmp &&\
		tar cf - $(XBMCPREFIX) | tar xf - &&\
		rm -rf $(LCLPREFIX)/include &&\
		rm -rf $(LCLPREFIX)/lib/pkgconfig &&\
		rm -f  $(LCLPREFIX)/lib/*.a &&\
		$(STRIP) --strip-unneeded `find . -name xbmc.bin -print` &&\
		$(STRIP) --strip-unneeded `find . -name *.so -print` \
	)
	tar -czvf $(TARBALL2) -C tmp .

clean:
	rm -rf tmp
	rm -f $(TARBALL1) $(DEB)
	rm -f $(TARBALL2)
