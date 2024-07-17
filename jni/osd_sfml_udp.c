#include <signal.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>

#include <SFML/Graphics.h>
#include <SFML/Window.h>

#include "msp/msp.h"
#include "msp/msp_displayport.h"
#include "net/serial.h"
#include "net/network.h"
#include "util/debug.h"

#include "lz4/lz4.h"
#include "net/network.h"
#include "net/data_protocol.h"

#define X_OFFSET 120

#define PORT 7654

#define WIDTH 1440
#define HEIGHT 810

#define DICTIONARY_VERSION 1

typedef struct display_info_s {
    uint8_t char_width;
    uint8_t char_height;
    uint8_t font_width;
    uint8_t font_height;
    uint16_t num_chars;
} display_info_t; 

#define SD_DISPLAY_INFO {.char_width = 31, .char_height = 15, .font_width = 36, .font_height = 54, .num_chars = 256}

static const display_info_t sd_display_info = SD_DISPLAY_INFO;

static const display_info_t hd_display_info = {
    .char_width = 50,
    .char_height = 18,
    .font_width = 24,
    .font_height = 36,
    .num_chars = 512,
};

#define MAX_OSD_WIDTH 60//50
#define MAX_OSD_HEIGHT 22//18

static display_info_t current_display_info = SD_DISPLAY_INFO;

static volatile sig_atomic_t quit = 0;
sfTexture *font_1;
sfTexture *font_2;
sfSprite *font_sprite_1;
sfSprite *font_sprite_2;
sfRenderWindow *window;
uint16_t character_map[MAX_OSD_WIDTH][MAX_OSD_HEIGHT];
displayport_vtable_t *display_driver;

static void sig_handler(int _)
{
    quit = 1;
}

static void draw_character(uint32_t x, uint32_t y, uint16_t c)
{
    if (x > current_display_info.char_width - 1 || y > current_display_info.char_height - 1)
    {
        return;
    }
    character_map[x][y] = c;
    /*for(int i=0;i<MAX_OSD_WIDTH;i++){
        for(int j=0;j<MAX_OSD_HEIGHT;j++){
            character_map[i][j]=(i*MAX_OSD_HEIGHT+j) % 255;
        }
    }*/
}

static void draw_screen()
{
    sfRenderWindow_clear(window, sfColor_fromRGB(55, 55, 100));
    for (int y = 0; y < current_display_info.char_height; y++)
    {
        for (int x = 0; x < current_display_info.char_width; x++)
        {
            uint16_t c = character_map[x][y];
            if (c != 0)
            {
                uint8_t page = 0;
                if (c > 255) {
                    page = 1;
                    c = c & 0xFF;
                }
                //DEBUG_PRINT("%02X", c);
                sfIntRect r = {0, current_display_info.font_height * c, current_display_info.font_width, current_display_info.font_height};
                sfVector2f dest = {(x * current_display_info.font_width) + X_OFFSET, y * current_display_info.font_height};
                sfSprite *font_sprite = page ? font_sprite_2 : font_sprite_1;
                sfSprite_setTextureRect(font_sprite, r);
                sfSprite_setPosition(font_sprite, dest);
                sfRenderWindow_drawSprite(window, font_sprite, NULL);
            }
            //DEBUG_PRINT("  ");
        }
        //DEBUG_PRINT("\n");
    }
}

static void clear_screen()
{
    DEBUG_PRINT("clear\n");
    memset(character_map, 0, sizeof(character_map));
}

static void draw_complete()
{
    draw_screen();
    sfRenderWindow_display(window);
    //DEBUG_PRINT("draw complete!\n");
}

static void msp_callback(msp_msg_t *msp_message)
{
    displayport_process_message(display_driver, msp_message);
}

static void feed_dummy(){
    msp_msg_t msg={};
    msg.direction=MSP_INBOUND;
    msg.cmd=MSP_CMD_DISPLAYPORT;
    msg.payload[0]=MSP_DISPLAYPORT_DRAW_SCREEN;
    msp_callback(&msg);
    msg.payload[0]=MSP_DISPLAYPORT_DRAW_STRING;
    msg.payload[1]=10;
    msg.payload[2]=10;
    msg.payload[3]=0;
    msg.payload[4]=99;
    msg.payload[5]='\0';
    msp_callback(&msg);

}

static void set_options(uint8_t font, uint8_t is_hd) {
    if(is_hd) { 
        current_display_info = hd_display_info;
    } else {
        current_display_info = sd_display_info;
    }
}

static void process_compressed_data(void *buf, int len, void *dict, int dict_size) {
    compressed_data_header_t *header = (compressed_data_header_t*)buf;
    if (header->version != DICTIONARY_VERSION) {
        return;
    }
   /* switch ((msp_hd_options_e)header->hd_options) {
        case MSP_HD_OPTION_60_22:
            fakehd_disable();
            current_display_info = &full_display_info;
            break;
        case MSP_HD_OPTION_30_16:
        case MSP_HD_OPTION_50_18:
            fakehd_disable();
            current_display_info = &hd_display_info;
            break;
        default:
            if (fakehd_is_enabled()) {
                memset(msp_render_character_map, 0, sizeof(msp_render_character_map));
                current_display_info = &full_display_info;
            } else {
                current_display_info = &sd_display_info;
            }
            break;
    }*/
    int decompressed_size = LZ4_decompress_safe_usingDict((buf + sizeof(compressed_data_header_t)), character_map, len - sizeof(compressed_data_header_t), sizeof(character_map), dict, dict_size);
    DEBUG_PRINT("Decompressed %d bytes!\n", decompressed_size);
    //msp_draw_complete();
}

int main(int argc, char *argv[])
{
    struct pollfd poll_fds[1];
    signal(SIGINT, sig_handler);
    memset(character_map, 0, sizeof(character_map));
    sfVideoMode videoMode = {1440, 810, 32};
    window = sfRenderWindow_create(videoMode, "MSP OSD", sfResize, NULL);
    sfVector2i position={500,500};
    sfRenderWindow_setPosition(window,position);
    sfRenderWindow_display(window);
    char *font_name;
    if (argc > 1) {
        font_name = argv[1];
    } else {
        font_name = "bold.png";
    }
    char font_load_name[255];
    snprintf(font_load_name, 255, "%s.png", font_name);
    //font_1 = sfTexture_createFromFile(font_name, NULL);
    font_1 = sfTexture_createFromFile("fonts/font_inav.png", NULL);
    font_sprite_1 = sfSprite_create();
    sfSprite_setTexture(font_sprite_1, font_1, 0);
    char font_2_name[255];
    snprintf(font_2_name, 255, "%s_2.png", font_name);
    font_2 = sfTexture_createFromFile(font_2_name, NULL);
    font_sprite_2 = sfSprite_create();
    sfSprite_setTexture(font_sprite_2, font_2, 0);

    display_driver = calloc(1, sizeof(displayport_vtable_t));
    display_driver->draw_character = &draw_character;
    display_driver->clear_screen = &clear_screen;
    display_driver->draw_complete = &draw_complete;
    display_driver->set_options = &set_options;

    int compression_dict_size = 0;
    void *compression_dict = open_dict(DICTIONARY_VERSION, &compression_dict_size);

    msp_state_t *msp_state = calloc(1, sizeof(msp_state_t));
    msp_state->cb = &msp_callback;

    //int socket_fd = bind_socket(PORT);
    int socket_fd = bind_socket(7656);
    //int socket_fd_compressed= bind_socket(PORT+1);
    int recv_len = 0;
    uint8_t buffer[4096];

    struct timespec fps_start, now;
    uint32_t message_counter = 0;
    clock_gettime(CLOCK_MONOTONIC, &fps_start);

    printf("started up, listening on port %d\n", PORT);

    while (!quit)
    {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if(now.tv_sec > fps_start.tv_sec) {
            clock_gettime(CLOCK_MONOTONIC, &fps_start);
            printf("Got %d messages in the last second\n", message_counter);
            message_counter = 0;
        }        

        sfEvent event;
        sfRenderWindow_pollEvent(window, &event);

        // Close window: exit
        if (event.type == sfEvtMouseButtonReleased)
        {
            sfRenderWindow_close(window);
            quit = 1;
        }

        poll_fds[0].fd = socket_fd;
        poll_fds[0].events = POLLIN;
        if(poll(poll_fds, 1, 50) > 0) // poll every 50ms so we also go back through the SFML loop  
        {
            struct sockaddr_storage src_addr;
            socklen_t src_addr_len=sizeof(src_addr);
            if (0 < (recv_len = recvfrom(socket_fd,&buffer,sizeof(buffer),0,(struct sockaddr*)&src_addr,&src_addr_len)))
            {
                printf("Got data %d\n",recv_len);
                message_counter++;
                //for (int i=0; i<recv_len; i++)
                //    msp_process_data(msp_state, buffer[i]);
                process_compressed_data(buffer,recv_len,compression_dict,compression_dict_size);
            }
        }
        feed_dummy();
    }
    sfRenderWindow_close(window);
    sfSprite_destroy(font_sprite_1);
    sfSprite_destroy(font_sprite_2);
    sfTexture_destroy(font_1);
    sfTexture_destroy(font_2);
    sfRenderWindow_destroy(window);
    free(msp_state);
    free(display_driver);
    close(socket_fd);
    return 0;
}