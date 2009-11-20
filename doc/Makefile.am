
dist_man_MANS = dradio-config.1 dradio.1 dradio.5 

changelog.txt: ../ChangeLog
	@sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/\æ/\&aelig;/g; s/\ø/\&oslash;/g; s/\å/\&aring;/g; s/\Æ/\&AElig;/g; s/\Ø/\&Oslash;/g; s/\Å/\&Aring;/g;' $(srcdir)/$< > $@;

