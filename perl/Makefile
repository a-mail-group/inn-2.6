##  $Id: Makefile 9303 2011-08-04 22:09:57Z iulius $

include ../Makefile.global

top	      = ..

ALL           = INN/Utils/Shlock.pm

EXTRA         = INN/Config.pm

##  The double underscore '__' will be converted into two colons '::'
##  during the install process (otherwise, make would complain in this
##  file, and we cannot generate the right file with '::' in the
##  Subversion man directory because it would always be regenerated:
##  the file containing '__' would indeed not be found).
MAN           = ../doc/man/INN__Config.3pm \
		../doc/man/INN__Utils__Shlock.3pm

all: $(ALL) $(EXTRA) $(MAN)

install: all
	for F in $(ALL) $(EXTRA) ; do \
	    $(CP_RPUB) $$F $D$(PATHLIBPERL)/$$F ; \
	done

bootstrap: $(MAN)

clean:
	rm -f $(ALL)

clobber distclean: clean
	rm -f $(EXTRA)

maintclean: distclean
	rm -f $(MAN)

depend:

profiled: all

$(EXTRA) $(FIXSCRIPT):
	@echo Run configure before running make.  See INSTALL for details.
	@exit 1

##  Compilation rules.

FIX		= $(FIXSCRIPT)

INN/Utils/Shlock.pm: INN/Utils/Shlock.pm.in	$(FIX) ; $(FIX) INN/Utils/Shlock.pm.in

../doc/man/INN__Config.3pm: INN/Config.pm.in
	$(POD2MAN) -s '$(MAN3PM_EXT)' -n "INN::Config" $? > $@

../doc/man/INN__Utils__Shlock.3pm: INN/Utils/Shlock.pm.in
	$(POD2MAN) -s '$(MAN3PM_EXT)' -n "INN::Utils::Shlock" $? > $@
