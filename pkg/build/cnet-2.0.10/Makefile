NAME		= cnet
VERSION		= 2.0.10

SHELL		= /bin/sh

# setup for Computer Science, The University of Western Australia:
#
PREFIX		= /usr
#PREFIX		= $(HOME)
#
BINDIR		= $(PREFIX)/bin
CNETDIR		= $(PREFIX)/share/cnet
MANDIR		= $(PREFIX)/share/man/man1
MANEXT		= 1
WEBDIR		= /var/www/apache2-default/$NAME
#WEBDIR		= $(HOME)/WWW/cnet

# ---------------------------------------------------------------------

it:
	(cd src && make)

install:
	(cd src && make)
	cp $(NAME) $(DESTDIR)$(BINDIR)/$(NAME) 
	chmod 755 $(DESTDIR)$(BINDIR)/$(NAME)
	mkdir -p $(DESTDIR)$(CNETDIR) && chmod 755 $(DESTDIR)$(CNETDIR)
	cp cnetlib/* $(DESTDIR)$(CNETDIR)
	chmod 644 $(DESTDIR)$(CNETDIR)/*
	cp DOC/$(NAME).man $(DESTDIR)$(MANDIR)/$(NAME).$(MANEXT)
	chmod 644 $(DESTDIR)$(MANDIR)/$(NAME).$(MANEXT)

html:
	(cd HTMLORIG && ./INSTALL)

web:
	make html
	mkdir -p $(WEBDIR)        && chmod 755 $(WEBDIR)
	mkdir -p $(WEBDIR)/images && chmod 755 $(WEBDIR)/images
	cp -p DOC/*.* $(WEBDIR)
	cp -p DOC/images/*.gif DOC/images/*.jpg $(WEBDIR)/images
	chmod 644 $(WEBDIR)/*.* $(WEBDIR)/images/*.*


# ---------------------------------------------------------------------
#
# setup for Computer Science, The University of Western Australia:
#
DISTDIR		= /home/www/DOCS/pls/cnet
IDEASDIR	= $(HOME)/WWW/cnet-extraideas
#
DISTFILES	= ANNOUNCE DOC EXAMPLES cnetlib src
DISTNAME	= $(NAME)-$(VERSION)
BACKUPDIR	= $(HOME)/backup

clean:
	(cd src && make clean)
	(cd DOC		    && make clean)
	(cd EXAMPLES	    && make clean)
	rm -rf $(NAME) $(DISTNAME) $(DISTNAME).tgz f?

backup:
	make clean
	tar zcf $(BACKUPDIR)/$(DISTNAME).tgz .

zip:
	make backup
	cp $(BACKUPDIR)/$(DISTNAME).tgz /zip ; sync ; ls -l /zip

dist:
	make clean
	make html
	groff -man DOC/cnet.man > DOC/cnet.ps
	mkdir $(DISTNAME)
	cp -R -p Makefile.dist $(DISTNAME)/Makefile
	cp -R -p $(DISTFILES) $(DISTNAME)
	rm -rf $(DISTNAME)/DOC/HTML
	(cd $(DISTNAME) && ln -s src/cnet.h . && ln -s src/cnet64bits.h . )
	tar zcf $(DISTNAME).tgz $(DISTNAME)
	chmod 644 $(DISTNAME).tgz
	cp -p $(DISTNAME).tgz $(DISTDIR)
	rm -rf $(DISTNAME) $(DISTNAME).tgz

ideas:
	rm -rf $(IDEASDIR)
	mkdir $(IDEASDIR) && chmod 711 $(IDEASDIR)
	cp -p -R IDEAS/* $(IDEASDIR)
	chmod 644 $(IDEASDIR)/*.* $(IDEASDIR)/images/*.*

