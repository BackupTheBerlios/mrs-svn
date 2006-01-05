# Makefile for website distribution

firstTarget: all

TARGET = mrsweb.tgz

DISTFILES = \
	cgi-bin/mrs.cgi \
	cgi-bin/Format.pm \
	cgi-bin/Format_*.pm \
	cgi-bin/mrs.conf.default \
	cgi-bin/help-*.html \
	htdocs/mrs.js \
	htdocs/mrs_style.css \
	htdocs/index.html \
	parsers/*.pm

all: $(TARGET)

$(TARGET): $(DISTFILES)
	tar cvzf $(TARGET) $(DISTFILES)

