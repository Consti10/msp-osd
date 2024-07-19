CC=gcc
CFLAGS=-I. -O2
SRCDIR = jni/
DEPS = $(addprefix $(SRCDIR), msp/msp.h msp/msp_displayport.h net/network.h net/serial.h lz4/lz4.h util/debug.h)
OSD_OBJ = $(addprefix $(SRCDIR), osd_sfml_udp.o net/network.o msp/msp.o msp/msp_displayport.o lz4/lz4.o util/fs_util.o)
DISPLAYPORT_MUX_OBJ = $(addprefix $(SRCDIR), msp_displayport_mux.o net/serial.o net/network.o msp/msp.o msp/msp_displayport.o lz4/lz4.o util/fs_util.o hw/dji_radio_shm.o)
X_DJI_UDP_OVERLAY_OBJ = $(addprefix $(SRCDIR), osd_dji_overlay_udp.o net/serial.o net/network.o msp/msp.o msp/msp_displayport.o lz4/lz4.o util/fs_util.o toast/toast.o fakehd/fakehd.o json/parson.o json/osd_config.o font/font.o  hw/dji_radio_shm.o hw/dji_display.o hw/dji_services.o rec/rec.o rec/rec_shim.o rec/rec_util.o rec/rec_pb.o libspng/spng.o)
OSD_LIBS=-lcsfml-graphics
X_DJI_UDP_OVERLAY_LIBS = -lz -lm -lSDL2

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

osd_sfml: $(OSD_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(OSD_LIBS)

msp_displayport_mux: $(DISPLAYPORT_MUX_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

osd_dji_overlay_udp: $(X_DJI_UDP_OVERLAY_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(X_DJI_UDP_OVERLAY_LIBS)

all: osd_sfml msp_displayport_mux osd_dji_overlay_udp

clean: 
	rm -rf *.o
	rm -rf **/*.o
	rm -f msp_displayport_mux
	rm -f osd_sfml
	rm -f osd_dji_overlay_udp
