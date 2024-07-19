#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#define EMULATE_DJI_GOGGLES

#ifndef EMULATE_DJI_GOGGLES
#include <sys/mman.h>
#endif

#include "dji_radio_shm.h"

void open_dji_radio_shm(dji_shm_state_t *shm) {
#ifndef EMULATE_DJI_GOGGLES
    int fd = open("/dev/mem", O_RDWR);
    assert(fd > 0);
    shm->mapped_address = mmap64(NULL, RTOS_SHM_SIZE, PROT_READ, MAP_SHARED, fd, RTOS_SHM_ADDRESS);
    assert(shm->mapped_address != MAP_FAILED);
    close(fd);
    shm->modem_info = (modem_shmem_info_t *)((uint8_t *)shm->mapped_address + 0x100);
    shm->product_info = (product_shm_info_t *)((uint8_t *)shm->mapped_address + 0xC0);
#else
    shm = calloc(1, sizeof(dji_shm_state_t));
#endif
}

void close_dji_radio_shm(dji_shm_state_t *shm) {
#ifndef EMULATE_DJI_GOGGLES
    munmap(shm->mapped_address, RTOS_SHM_SIZE);
    shm->mapped_address = NULL;
    shm->modem_info = NULL;
    shm->product_info = NULL;
#else
    free(shm);
#endif
}

uint16_t dji_radio_latency_ms(dji_shm_state_t *shm) {
    return shm->product_info->frame_delay_e2e;
}

uint16_t dji_radio_mbits(dji_shm_state_t *shm) {
    return shm->modem_info->channel_status;
}