all: debpkg

debpkg: vsa btns
	cp vsa debpkg/usr/bin/vsa
	cp vsa_btns debpkg/usr/bin/vsa_btns
	cp known_keys.cfg debpkg/etc/vsa/known_keys.cfg
	cp button_actions.cfg debpkg/etc/vsa/button_actions.cfg
	cp init.sh debpkg/etc/init.d/vsa_btns
	chmod 755 debpkg/DEBIAN/postinst
	dpkg --build debpkg "vsa_arm64.deb"

vsa: vsa.c style.css vsalib
	xxd -i style.css > style.h
	gcc -o vsa vsa.c vsalib.o `pkg-config --cflags --libs gtk+-3.0`

btns: vsa_btns.c vsalib
	gcc -o vsa_btns vsa_btns.c vsalib.o

vsalib: vsalib.c
	gcc -c vsalib.c

clean:
	rm -f *.deb
	rm -f debpkg/usr/bin/vsa
	rm -f debpkg/usr/bin/btns
	rm -f debpkg/etc/vsa/known_keys.cfg
	rm -f debpkg/etc/vsa/button_actions.cfg
	rm -f debpkg/etc/init.d/vsa_btns
	rm -f vsa
	rm -f vsa_btns
	rm -f vsalib.o
	rm -f style.h
