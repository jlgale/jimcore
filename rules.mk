CC = /usr/bin/gcc

WARNINGS = \
	-Wall -Wextra \
	-Wmissing-declarations \
	-Wno-unused-parameter #-Wno-unused-function
DEBUGGING = -ggdb
ifndef DEBUG
OPTIMIZATIONS = -O2 -DNDEBUG
endif
DEFINES += \
	LOG_WITH_POSITION \
#
LIBS += \
	libz \
	librt \

INCLUDE += .

CFLAGS = -std=gnu99 -Werror $(WARNINGS) $(OPTIMIZATIONS) $(DEBUGGING)\
	$(patsubst %, -D%, $(DEFINES)) -pthread $(patsubst %, -I%, $(INCLUDE))\
	$(shell mysql_config --include)

LDFLAGS = -pthread $(patsubst lib%, -l%, $(LIBS)) $(patsubst %, -L%, $(LINK)) \
	$(shell mysql_config --libs)

#TOOLCHAIN_BUILD = 1
ifeq ($(TOOLCHAIN_BUILD), 1)
KROBIX = /opt/krobix/tools
PATH = $(KROBIX)/bin
LDFLAGS += -Wl,--dynamic-linker,/lib/ld-linux-x86-64.so.2
LINK += $(KROBIX)/lib/mysql
else
LINK += /usr/lib64/mysql
endif

.PHONEY: etags all_clean

all_clean: clean
	$(RM) .*.mk TAGS  core.* vgcore.*

.%.mk: %.c
	$(CC) -o $@ -M -MM -MG $(CFLAGS) $<
	$(CC) -M -MG -MM -MT $@ $(CFLAGS) $< >> $@

etags: 
	/usr/bin/ctags -e *.c *.h

ifneq ($(MAKECMDGOALS),clean)
include $(patsubst %.o, .%.mk, $(OFILES))
endif
