#
# Makefile for generating AmigaGuide reader documentation
# Formatting is done using XSL stylesheet and Sablotron processor
# Written by T.Pierron
#

CC=sabcmd
XML=DocAGR.xml
HTML=agr.html
GUIDE=agr.guide
READ=README

all: $(GUIDE) $(HTML) $(READ)

# Remove first line! [<?xml version=...?>]
$(GUIDE): xml2guide.xsl $(XML)
	$(CC) $< $(XML) | sed -e 1d >$@

$(READ): xml2readme.xsl $(XML)
	$(CC) $< $(XML) | sed -e 1d >$@

$(HTML): xml2html.xsl $(XML)
	$(CC) $< $(XML) $@

install: all
	mv $(GUIDE) ../../Docs/
	mv $(HTML)  ../../Docs/
	mv $(READ)  ../../
	cp Snap.IFF ../../Docs/

clean:
	rm -f $(GUIDE) $(HTML) $(READ)

