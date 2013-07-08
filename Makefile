INTERFACE = \
	counter.h \
	connection.h \
	histogram.h \
	logging.h \
	lru.h \
	mysql_helpers.h \
	ring.h \
	sys_linker_set.h \
	utils.h \

#
OFILES = \
	connection.o \
	counter.o \
	debug.o \
	histogram.o \
	logging.o \
	lru.o \
	pool.o \

#


.PHONEY: all clean

all: libjimcore.a $(patsubst %, jimcore/%, $(INTERFACE))

include rules.mk

jimcore:
	mkdir -p $@

jimcore/%.h: %.h jimcore
	cp -f $< $@

libjimcore.a: $(OFILES)
	$(AR) rcs $@ $^

clean:
	$(RM) *.o libjimcore.a
	$(RM) -r jimcore
