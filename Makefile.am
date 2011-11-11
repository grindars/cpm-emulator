SUBDIRS      = src rom
EXTRA_DIST   = generate-gitfiles.sh

dist-hook:
	sh $(top_srcdir)/generate-gitfiles.sh $(top_srcdir) $(distdir) 
