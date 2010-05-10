.DELETE_ON_ERROR:
.PHONY: all test clean mrproper
.DEFAULT_GOAL:=test

TAGBIN:=ctags
MARCH:=native

# Avoid unnecessary uses of 'pwd'; absolute paths aren't as robust as relative
# paths against overlong total path names.
OUT:=out
SRCDIR:=src
CSRCDIRS:=$(wildcard $(SRCDIR)/*)

PTRACER:=ptracer
BINS:=$(OUT)/$(PTRACER)

# We don't want to have to list all our source files, so discover them based on
# the per-language directory specifications above.
CSRC:=$(shell find $(CSRCDIRS) -type f -name \*.c -print)
CINC:=$(shell find $(CSRCDIRS) -type f -name \*.h -print)
COBJ:=$(addprefix $(OUT)/,$(addsuffix .o,$(basename $(CSRC))))
SRC:=$(CSRC)
TAGS:=.tags

# Anything that all source->object translations ought dep on. We currently
# include all header files in this list; it'd be nice to refine that FIXME.
GLOBOBJDEPS:=$(CINC) $(MAKEFILE_LIST) $(TAGS)

# Debugging flags. Normally unused, but uncomment the 2nd line to enable.
DEBUGFLAGS:=-rdynamic -g -D_FORTIFY_SOURCE=2
DFLAGS+=$(DEBUGFLAGS)

# Main compilation flags. Define with += to inherit from system-specific flags.
IFLAGS+=-I$(SRCDIR)
MFLAGS:=-march=$(MARCH)
ifdef MTUNE
MFLAGS+=-mtune=$(MTUNE)
endif
# Not using: -Wpadded, -Wconversion, -Wstrict-overflow=(>1)
WFLAGS+=-Wall -W -Wextra -Werror -Wmissing-prototypes -Wundef -Wshadow \
        -Wstrict-prototypes -Wmissing-declarations -Wnested-externs \
        -Wsign-compare -Wpointer-arith -Wbad-function-cast -Wcast-qual \
        -Wdeclaration-after-statement -Wfloat-equal -Wpacked -Winvalid-pch \
        -Wdisabled-optimization -Wcast-align -Wformat -Wformat-security \
        -Wold-style-definition -Woverlength-strings -Wwrite-strings \
	-Wstrict-aliasing=3 -Wunsafe-loop-optimizations -Wstrict-overflow=1
# We get the following from -O (taken from gcc 4.3 docs)
# -fauto-inc-dec -fcprop-registers -fdce -fdefer-pop -fdelayed-branch -fdse \
# -fguess-branch-probability -fif-conversion2 -fif-conversion \
# -finline-small-functions -fipa-pure-const -fipa-reference -fmerge-constants \
# -fsplit-wide-types -ftree-ccp -ftree-ch -ftree-copyrename -ftree-dce \
# -ftree-dominator-opts -ftree-dse -ftree-fre -ftree-sra -ftree-ter -ftree-sink \
# -funit-at-a-time, "and -fomit-frame-pointer on machines where it doesn't
# interfere with debugging."

# We add the following with -O2 (taken from gcc 4.3 docs)
# -fthread-jumps -falign-functions  -falign-jumps -falign-loops -falign-labels \
# -fcaller-saves -fcrossjumping -fcse-follow-jumps  -fcse-skip-blocks \
# -fdelete-null-pointer-checks -fexpensive-optimizations -fgcse -fgcse-lm \
# -foptimize-sibling-calls -fpeephole2 -fregmove -freorder-blocks \
# -freorder-functions -frerun-cse-after-loop -fsched-interblock -fsched-spec \
# -fschedule-insns -fschedule-insns2 -fstrict-aliasing -fstrict-overflow \
# -ftree-pre -ftree-store-ccp -ftree-vrp

# -O3 gets the following:
# -finline-functions -funswitch-loops -fpredictive-commoning -ftree-vectorize \
# -fgcse-after-reload

# The following aren't bound to any -O level:
# -fipa-pta -fipa-cp -ftree-loop-linear -ftree-loop-im -ftree-loop-ivcanon \
# -funsafe-loop-optimizations
# The following also require profiling info:
# -fipa-matrix-reorg
# These require -pthread:
# -ftree-parallelize-loops
OFLAGS+=-O2 -fomit-frame-pointer -finline-functions -fdiagnostics-show-option \
	-fvisibility=hidden -fipa-cp -ftree-loop-linear -ftree-loop-im \
	-ftree-loop-ivcanon -fno-common -ftree-vectorizer-verbose=5
#OFLAGS+=-fdump-tree-all
CFLAGS+=-pipe -std=gnu99 $(DFLAGS)
MT_CFLAGS:=$(CFLAGS) -pthread $(MT_DFLAGS)
CFLAGS+=$(IFLAGS) $(MFLAGS) $(OFLAGS) $(WFLAGS)
MT_CFLAGS+=$(IFLAGS) $(MFLAGS) $(OFLAGS) $(WFLAGS)
LIBFLAGS+=-lpthread
LFLAGS+=-Wl,-O,--default-symver,--enable-new-dtags,--as-needed,--warn-common \
	-Wl,--fatal-warnings,-z,noexecstack,-z,combreloc

all: test

test: $(TAGS) $(BINS)
	$(OUT)/$(PTRACER) -- /bin/ls
	$(OUT)/$(PTRACER) -- /bin/ls
	$(OUT)/$(PTRACER) -- /bin/ls --color
	# goes into an infinite loop FIXME
	#$(OUT)/$(PTRACER) -- $(OUT)/$(PTRACER) -- /bin/ls --color

$(OUT)/$(PTRACER): $(COBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

# Generic rules
$(OUT)/%.o: %.c $(GLOBOBJDEPS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble only, sometimes useful for close-in optimization
$(OUT)/%.s: %.c $(GLOBOBJDEPS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -fverbose-asm -S $< -o $@

# Preprocess only, sometimes useful for debugging
$(OUT)/%.i: %.c $(GLOBOBJDEPS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -E $< -o $@

$(TAGS): $(SRC) $(CINC) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(TAGBIN) -f $@ $^

clean:
	rm -rf $(OUT)

mrproper: clean
	rm -rf $(TAGS)
