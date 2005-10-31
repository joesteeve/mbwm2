LIBS=`pkg-config --libs x11`
INCS=`pkg-config --cflags x11`

#LIBS=`pkg-config --libs xcb`
#INCS=`pkg-config --cflags xcb`

OBJS=mb-wm-core.o mb-wm-stack.o mb-wm-client.o mb-wm-util.o mb-wm-atoms.o \
     mb-wm-props.c mb-wm-layout-manager.c xas.c \
     matchbox-window-manager-2.c

HEADERS=mb-wm.h mb-wm-client.h  mb-wm-stack.h mb-wm-util.h mb-wm-types.h \
	mb-wm-atoms.h mb-wm-types.h mb-wm-core.h xas.h mb-wm-layout-manager.h

.c.o:
	$(CC) -g -Wall $(CFLAGS) $(INCS) -c $*.c


matchbox-window-manager-2: $(OBJS)
	$(CC) -g -Wall $(CFLAGS) -o $@ $(OBJS) $(LIBS)


$(OBJS): $(HEADERS)

clean:
	rm -fr *.o matchbox-window-manager


