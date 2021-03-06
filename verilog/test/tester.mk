include ../../Design.mk

#SOURCES:=hrmcpu.v ufifo.v ALU.v MEMORY.v register.v IR.v program.v PC.v ControlUnit.v ram.v test/tester.v
SOURCES:= $(DEPS_HRMCPU) hrmcpu.v test/tester.v

AUXFILE:=program ram.tmp

.DEFAULT_GOAL := all

MAKEFLAGS += -j 8

#.SILENT:

# Add ../ to list of dependency files:
SRCFILEPATH:=$(addprefix ../../, $(SOURCES))

# Directory name we are in now...
LEVEL_DIR := $(strip $(notdir $(patsubst %/,%,$(CURDIR))))

-include $(LEVEL_DIR).mk

RED      = \033[0;31m
GREEN    = \033[0;32m
YELLOW   = \033[0;33m
BLUE     = \033[0;34m
NO_COLOR = \033[m

all-tests := $(addsuffix .check, $(basename $(wildcard *.in)))

.PHONY : test all #%.test_out %.ivl %.check

test : $(all-tests)

# Do not remove these files at the end
#.PRECIOUS: %.ivl %.test_out 

%.ivl : $(SRCFILEPATH) $(AUXFILE)
	iverilog $(SRCFILEPATH) -DPROGRAM=\"program\" -DROMFILE=\"ram.tmp\" -DINBFILE=\"$*.in\" $(IVERILOG_OPT) -DDUMPFILE=\"$*.lxt\" -o $*.ivl

ifneq ("$(wildcard ram)","")
ram.tmp: ram
	cp ram ram.tmp
else
ram.tmp:
	../../../hrmasm/hrmasm -r ram.tmp
endif


program: PROG
	../../../hrmasm/hrmasm -s PROG -o program -p -

%.test_out : %.in %.ivl
	vvp $*.ivl -lxt > $@

%.check : %.test_out %.out
	grep "TX OUT:" $*.test_out | awk '{ print $$4 }' | diff -q $*.out - >/dev/null && \
	( touch $*.check; printf "%b" "$(GREEN)$(LEVEL_DIR): Test [$*] OK$(NO_COLOR)\n" ) || \
	( printf "%b" "$(RED)$(LEVEL_DIR): Test [$*] FAILED$(NO_COLOR)\n" ; false )

all : test 
	@printf "%b" "$(GREEN)$(LEVEL_DIR): Success, all tests passed$(NO_COLOR)\n"

clean :
	-rm -f *.ivl *.check *.vcd *.lxt *.test_out program ram.tmp
