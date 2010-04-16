#  Canonical Linux Ubuntu Usplash theme
#  based on a design by Vadin Bu aka panoptus
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#  Oct. 04, 2009
#  Esteban Torres <etorres2+public@gmail.com>
#
#  Adapted for Kuliax, April 2010

CC=gcc
CFLAGS=-g -Wall -fPIC
LDFLAGS=
INCLUDES=

COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(CFLAGS) $(LDFLAGS)

INSTALL = /usr/bin/install -c
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_PROGRAM = $(INSTALL) -m 755

THEMENAME = kuliax-theme

$(THEMENAME).so: image_640_480.png.c.o \
                 image_800_600.png.c.o \
                 image_1024_640.png.c.o \
                 image_1024_768.png.c.o \
                 image_1280_720.png.c.o \
                 image_1280_800.png.c.o \
                 image_1280_1024.png.c.o \
                 image_1440_900.png.c.o \
                 image_1600_1200.png.c.o \
                 image_1680_1050.png.c.o \
                 progress_fore_640_480.png.c.o \
                 progress_fore_800_600.png.c.o \
                 progress_fore_1024_640.png.c.o \
                 progress_fore_1024_768.png.c.o \
                 progress_fore_1280_720.png.c.o \
                 progress_fore_1280_800.png.c.o \
                 progress_fore_1280_1024.png.c.o \
                 progress_fore_1440_900.png.c.o \
                 progress_fore_1600_1200.png.c.o \
                 progress_fore_1680_1050.png.c.o \
                 progress_back_640_480.png.c.o \
                 progress_back_800_600.png.c.o \
                 progress_back_1024_640.png.c.o \
                 progress_back_1024_768.png.c.o \
                 progress_back_1280_720.png.c.o \
                 progress_back_1280_800.png.c.o \
                 progress_back_1280_1024.png.c.o \
                 progress_back_1440_900.png.c.o \
                 progress_back_1600_1200.png.c.o \
                 progress_back_1680_1050.png.c.o \
                 7x13B.bdf.c.o \
                 7x14B.bdf.c.o \
                 9x15B.bdf.c.o \
                 $(THEMENAME).c.o
	$(COMPILE) -shared -o $@ $^
	
%.png.c: %.png
	pngtousplash $< > $@

%.bdf.c: %.bdf
	bdftousplash $< > $@

%.c.o: %.c
	$(COMPILE) -o $@ -c $<

widescreen:
	rm -f normal
	mv ./image*png ./backup/
	mv ./wide/*png ./
	touch widescreen
normal:
	rm -f widescreen
	mv ./image*png ./wide/
	mv ./backup/*png ./
	touch normal
change:
	update-alternatives --config usplash-artwork.so
	mkinitramfs -o /boot/initrd.img-`uname -r`
install:
	$(INSTALL) -d $(DESTDIR)/usr/lib/usplash
	$(INSTALL_PROGRAM) $(THEMENAME).so $(DESTDIR)/usr/lib/usplash/$(THEMENAME).so
uninstall:
	rm -f $(DESTDIR)/usr/lib/usplash/$(THEMENAME).so
	update-alternatives --remove usplash-artwork.so $(DESTDIR)/usr/lib/usplash/$(THEMENAME).so
	update-alternatives --config usplash-artwork.so
	mkinitramfs -o /boot/initrd.img-`uname -r`
clean:
	rm -f *.png.c *.bdf.c *.c.o *.so
