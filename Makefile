##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

PREFIX		?= arm-none-eabi
#PREFIX		?= arm-elf

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
# Do not print "Entering directory ...".
MAKEFLAGS += --no-print-directory
endif

all: build

build: lib examples

lib:
	$(Q)$(MAKE) -C libopencm3

EXAMPLE_DIRS:=$(sort $(dir $(wildcard $(addsuffix /*/*/Makefile,$(addprefix examples/,$(TARGETS))))))
$(EXAMPLE_DIRS): lib
	@printf "  BUILD   $@\n";
	$(Q)$(MAKE) --directory=$@

examples: $(EXAMPLE_DIRS)
	$(Q)true

doc:
	$(Q)$(MAKE) -C doc html

# Bleh http://www.makelinux.net/make3/make3-CHP-6-SECT-1#make3-CHP-6-SECT-1
clean: cleanheaders
	$(Q)for i in $(LIB_DIRS) \
		     $(EXAMPLE_DIRS); do \
		if [ -d $$i ]; then \
			printf "  CLEAN   $$i\n"; \
			$(MAKE) -C $$i clean SRCLIBDIR=$(SRCLIBDIR) || exit $?; \
		fi; \
	done

.PHONY: build lib examples $(EXAMPLE_DIRS) install doc clean

