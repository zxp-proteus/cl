if ENABLE_GETTEXT
   LOCALE_DIR = locale
endif

SUBDIRS = src lisp-utils tests doc interfaces share demo plotting $(LOCALE_DIR)

EXTRA_DIST = \
  common.mk maxima.iss.in \
  README.lisps README.rpms README.i18n README.external \
  INSTALL.win32 INSTALL.lisp \
  configure.lisp  maxima-local.in xmaxima-local.in \
  ChangeLog-5.9.2 ChangeLog-5.9.3 ChangeLog-5.10.0 ChangeLog-5.11.0 \
  ChangeLog-5.12.0 ChangeLog-5.13.0 ChangeLog-5.14.0 ChangeLog-5.15.0 \
  ChangeLog-5.16 ChangeLog-5.17 ChangeLog-5.17-special-functions \
  ChangeLog-5.18 ChangeLog-5.19

include common.mk

extradocinstall:
	test -d "$(docdir)" || mkdir -p "$(docdir)"
	for file in AUTHORS COPYING LICENSES/getopt-license.txt INSTALL README README.lisps ; \
            do \
            $(INSTALL_DATA) $$file "$(docdir)" ; \
            done

$(distdir).tar.gz: Makefile.am
	$(MAKE) dist

iss: extradocinstall
	$(MAKE) -C interfaces/xmaxima/win32 gcccopy install

# Build the Maxima rpms. Optionally, use
#     make rpm SUDO=sudo
# to use sudo to wrap the commands. The user must have sudo permissions
# for rpm and cp.
rpm: $(distdir).tar.gz
	$(SUDO) cp $(distdir).tar.gz /usr/src/redhat/SOURCES && \
	$(SUDO) rpmbuild -ba maxima.spec
