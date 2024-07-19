#pragma once

#define EMULATE_DJI_GOGGLES

#ifndef EMULATE_DJI_GOGGLES
#include "duml_hal.h"
#else
#define duss_disp_plane_id_t uint8_t
#define duss_disp_instance_handle_t uint8_t
#define duss_hal_obj_handle_t uint8_t
#endif
#include <stdint.h>

typedef struct dji_display_state_s {
#ifndef EMULATE_DJI_GOGGLES
    duss_disp_plane_id_t plane_id;
    duss_hal_obj_handle_t disp_handle;
    duss_hal_obj_handle_t ion_handle;
    duss_disp_vop_id_t vop_id;
    duss_hal_mem_handle_t ion_buf_0;
    duss_hal_mem_handle_t ion_buf_1;
    void * fb0_virtual_addr;
    void * fb0_physical_addr;
    void * fb1_virtual_addr;
    void * fb1_physical_addr;
    duss_disp_instance_handle_t *disp_instance_handle;
    duss_frame_buffer_t *fb_0;
    duss_frame_buffer_t *fb_1;
    duss_disp_plane_blending_t *pb_0;
#endif
    uint8_t is_v2_goggles;
    uint8_t frame_drawn;
} dji_display_state_t;

void dji_display_push_frame(dji_display_state_t *display_state);
void dji_display_open_framebuffer(dji_display_state_t *display_state, duss_disp_plane_id_t plane_id);
void dji_display_open_framebuffer_injected(dji_display_state_t *display_state, duss_disp_instance_handle_t *disp, duss_hal_obj_handle_t ion_handle, duss_disp_plane_id_t plane_id);
void dji_display_close_framebuffer(dji_display_state_t *display_state);
dji_display_state_t *dji_display_state_alloc(uint8_t is_v2_goggles);
void dji_display_state_free(dji_display_state_t *display_state);
void *dji_display_get_fb_address(dji_display_state_t *display_state);

int sdl2_check_for_termination();
