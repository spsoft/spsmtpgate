
include ../port/port.mk

#--------------------------------------------------------------------

LIBEVENT_LIB  = -L$(HOME)/libevent -levent -lrt

LDFLAGS += $(LIBEVENT_LIB)

CFLAGS += -I../

LDFLAGS += -static -L../spserver -lspserver -L../spnetkit -lspnetkit \
		-L../spmime -lspmime \
		$(LIBEVENT_LIB) -lpthread -lstdc++ -lresolv

OBJS = spsgconfig.o spsgmsg.o spsgmilter.o \
	spsgbackend.o spsghandler.o spsmtpgate.o

TARGET = spsmtpgate mailload fakesmtp

#--------------------------------------------------------------------

all: $(TARGET)

spsmtpgate: $(OBJS)
	$(LINKER) -o $@ $^ $(LDFLAGS)

mailload: mailload.o
	$(LINKER) -o $@ $^ -lpthread

fakesmtp: fakesmtp.o
	$(LINKER) -o $@ $^ $(LDFLAGS)

fakemilter: fakemilter.o
	$(LINKER) -o $@ $^ -lpthread -lmilter

clean:
	@( $(RM) *.o vgcore.* core core.* $(TARGET) fakemilter )

