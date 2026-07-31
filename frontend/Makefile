# Install and test the Acus sugar frontend.

INSTALL ?= install
RM      ?= rm -f
RM_RF   ?= rm -rf

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DESTDIR ?=

PROGRAM := acs
INSTALL_PROGRAM := $(DESTDIR)$(BINDIR)/$(PROGRAM)

EXAMPLE_DIR       := examples
EXAMPLE_BUILD_DIR ?= build/examples
EXAMPLE_ACS_FLAGS ?= --acus-root ..

MAIN_BODY_EXAMPLE := $(EXAMPLE_DIR)/10_main_body.acs
PROGRAM_EXAMPLES  := $(filter-out $(MAIN_BODY_EXAMPLE),$(wildcard $(EXAMPLE_DIR)/*.acs))
PROGRAM_OUTPUTS   := $(patsubst $(EXAMPLE_DIR)/%.acs,$(EXAMPLE_BUILD_DIR)/%.bf,$(PROGRAM_EXAMPLES))
MAIN_BODY_OUTPUT  := $(EXAMPLE_BUILD_DIR)/10_main_body.bf

.PHONY: all install uninstall check examples clean-examples

all:
	chmod +x $(PROGRAM)

install: all
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 755 $(PROGRAM) $(INSTALL_PROGRAM)

uninstall:	
	$(RM) $(INSTALL_PROGRAM)

check:
	python3 -m py_compile $(PROGRAM)
	./$(PROGRAM) --help >/dev/null

examples: all $(PROGRAM_OUTPUTS) $(MAIN_BODY_OUTPUT)

$(EXAMPLE_BUILD_DIR)/%.bf: $(EXAMPLE_DIR)/%.acs $(PROGRAM)
	mkdir -p $(EXAMPLE_BUILD_DIR)
	./$(PROGRAM) $< $(EXAMPLE_ACS_FLAGS) -o $@

$(MAIN_BODY_OUTPUT): $(MAIN_BODY_EXAMPLE) $(PROGRAM)
	mkdir -p $(EXAMPLE_BUILD_DIR)
	./$(PROGRAM) $< --main-body $(EXAMPLE_ACS_FLAGS) -o $@

clean-examples:
	$(RM_RF) $(EXAMPLE_BUILD_DIR)
