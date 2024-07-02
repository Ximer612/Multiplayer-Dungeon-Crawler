#include <SDL.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

enum MESSAGE_TYPE {MESSAGE_JOIN, MESSAGE_WELCOME, MESSAGE_ACK, MESSAGE_POSITION, MESSAGE_MELEE};

// Struct declaration
typedef struct actor{
    unsigned int player_id;
    unsigned int last_message_counter;
    float x;
    float y;
    float current_x;
    float current_y;
    float last_x;
    float last_y;
    int velocity;
    int tile_x;
    int tile_y;
}actor;

typedef struct maze_packet
{
    unsigned int message_type;
    unsigned int message_counter;
    unsigned int player_id;
    float x;
    float y;
} maze_packet;

typedef struct ack_packet
{
    unsigned int message_id;
    unsigned int timestamp;
    unsigned int content;
} ack_packet;

// Fucntion declaration
int get_tile(const int offsetX, const int offsetY, const int width, const int height, int* xCoord, int *yCoord);
int draw_tile(SDL_Renderer *renderer, SDL_Texture *texture, const int tile_x, const int tile_y, const float x, const float y);
int draw_actor(SDL_Renderer *renderer, SDL_Texture *texture, actor *actor);
int load_font(SDL_Renderer *renderer, const char *path);
int draw_text(SDL_Renderer *renderer, const float x, const float y, const char *str);
int draw_room(SDL_Renderer* renderer, SDL_Texture* texture, const float room_x, const float room_y, const unsigned char bitmask);
int send_packet(const int s, const unsigned int message_type, const unsigned int p_id, const float x, const float y, const unsigned int message_counter, struct sockaddr_in *sin);
int send_acknowledge(const int s, const unsigned int message_id, const unsigned int timestamp, const unsigned int content, struct sockaddr_in *sin);
int init_socket(struct sockaddr_in *sin);
int receive_packet(const int s, char *buffer, const int len);
