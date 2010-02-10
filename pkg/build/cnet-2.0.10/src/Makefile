NAME		= cnet
SHELL		= /bin/sh

# ---------------------------------------------------------------------

it:
	@if uname -s -r | grep "Linux" ; then \
		make -f Makefile.linux $(NAME) ; fi
	@if uname -s -r | grep "SunOS 4" ; then \
		make -f Makefile.sunos $(NAME) ; fi
	@if uname -s -r | grep "SunOS 5" ; then \
		make -f Makefile.solaris $(NAME) ; fi
	@if uname -s -r | grep "OSF1" ; then \
		make -f Makefile.osf1 $(NAME) ; fi
	@if uname -s -r | grep "IRIX" ; then \
		make -f Makefile.irix5 $(NAME) ; fi
	@if uname -s -r | grep "FreeBSD" ; then \
		make -f Makefile.freebsd $(NAME) ; fi
	@if uname -s -r | grep "NetBSD" ; then \
		make -f Makefile.netbsd $(NAME) ; fi
	cp $(NAME) ../$(NAME)

# ---------------------------------------------------------------------

clean:
	rm -f $(NAME) *.o core *.cnet f?
	if [ -d libelf/libelf-0.6.4 ] ; then \
	    (cd libelf/libelf-0.6.4  && make clean) ; fi

hide:
	echo '#if	HIDE_GLOBAL_NAMES'		 > hidenames.h
	sort < HIDENAMES | \
	awk '{ if($$0 !~ /#/) printf("#define\t%-36s\t__CN%03d\n", $$0,++n);}'\
							>> hidenames.h
	echo '#endif'					>> hidenames.h

