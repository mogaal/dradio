
dist_man_MANS = dradio-config.1 dradio.1 dradio.5 

changelog.txt: ../ChangeLog
	@sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/\�/\&aelig;/g; s/\�/\&oslash;/g; s/\�/\&aring;/g; s/\�/\&AElig;/g; s/\�/\&Oslash;/g; s/\�/\&Aring;/g;' $(srcdir)/$< > $@;

