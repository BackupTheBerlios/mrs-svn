# Main makefile

include make.config

all:
	$(MAKE) -Cplugin all
	$(MAKE) -Cws/blast all
	$(MAKE) -Cws/regular all
	$(MAKE) -Cws/clustal all

clean:
	$(MAKE) -Cplugin clean
	$(MAKE) -Cws/blast clean
	$(MAKE) -Cws/regular clean
	$(MAKE) -Cws/clustal clean

install:
	install -d $(DATADIR)
	install -d $(P_SCRIPTDIR)
	find update_scripts | cpio -p $(U_SCRIPTDIR)
	install -d $(U_SCRIPTDIR)
	cat update_scripts/make_TEMPLATE.conf | \
		sed -e 's|__DATA_DIR__|$(DATADIR)|' \
			-e 's|__BIN_DIR__|$(BINDIR)|' \
			-e 's|__MAINTAINER__|$(MAINTAINER)|' \
			-e 's|__SCRIPT_DIR__|$(U_SCRIPTDIR)|' \
			-e 's|__PARSER_DIR__|$(P_SCRIPTDIR)|' \
			-e 's|__PERL__|$(PERL)|' \
			-e 's|__CXX__|$(CC)|' \
			> $(U_SCRIPTDIR)/make_$(HOST).conf
	find parser_scripts | cpio -p $(P_SCRIPTDIR)
	install -d $(PREFIX)/etc
	cat ws/mrs-config.xml.TEMPLATE | \
		sed -e 's|__DATA_DIR__|$(DATADIR)|' \
			-e 's|__PARSER_DIR__|$(P_SCRIPTDIR)|' \
			-e 's|__CLUSTALW__|$(CLUSTALW)|' \
			> $(PREFIX)/etc/mrs-config.xml
	$(MAKE) -Cplugin install
	$(MAKE) -Cws/blast install
	$(MAKE) -Cws/regular install
	$(MAKE) -Cws/clustal install

