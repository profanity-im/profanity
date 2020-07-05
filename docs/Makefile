#
# Manpage to build docs.
#

ADOC_FILE = profanity.1.adoc

all: manpage

manpage:
	asciidoc -b docbook $(ADOC_FILE)
	asciidoc -b html $(ADOC_FILE)
	a2x --doctype manpage --format manpage $(ADOC_FILE)

