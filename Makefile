CC=gcc
CFLAGS=-I. -O2
SRCDIR = jni/
DEPS = $(addprefix $(SRCDIR), msp/msp.h msp/msp_displayport.h net/network.h net/serial.h lz4/lz4.h )
OSD_OBJ = $(addprefix $(SRCDIR), osd_sfml_udp.o net/network.o msp/msp.o msp/msp_displayport.o lz4/lz4.o util/fs_util.o)
DISPLAYPORT_MUX_OBJ = $(addprefix $(SRCDIR), msp_displayport_mux.o net/serial.o net/network.o msp/msp.o msp/msp_displayport.o lz4/lz4.o util/fs_util.o hw/dji_radio_shm.o)
OSD_LIBS=-lcsfml-graphics

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

osd_sfml: $(OSD_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(OSD_LIBS)

msp_displayport_mux: $(DISPLAYPORT_MUX_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

all: osd_sfml msp_displayport_mux

clean: 
	rm -rf *.o
	rm -rf **/*.o
	rm -f msp_displayport_mux
	rm -f osd_sfml
