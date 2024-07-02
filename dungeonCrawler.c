#include <SDL.h>
#include "dungeon.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "maze.h"

extern float scale;
extern float camera_x;
extern float camera_y;
extern int draw_tile_counter;

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("Error initializing SDL2: %s", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, 0);

    if (!window)
    {
        SDL_Log("Error creating SDL2 Window: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("Error creating SDL2 Renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Log("Hello World!");

    int window_opened = 1;

    // Basic usage (see HDR discussion below for HDR usage):
    int width, height, channels;

    unsigned char* pixels = stbi_load("2D Pixel Dungeon Asset Pack/character and tileset/Dungeon_Tileset.png", &width, &height, &channels, 0);

    if (pixels == NULL)
    {
        SDL_Log("Error loading image");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Log("Image size: W %d, H %d C: %d\n", width, height, channels);

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);

    if (texture == NULL)
    {
        SDL_Log("Error creating SDL2 Texture: %s", SDL_GetError());
        stbi_image_free(pixels);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    if (SDL_UpdateTexture(texture, NULL, pixels, width * 4) != 0)
    {
        SDL_Log("Error update texture: %s", SDL_GetError());
        stbi_image_free(pixels);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    stbi_image_free(pixels);

    actor first;
    first.x = 0;
    first.y = 0;
    first.tile_x = 9;
    first.tile_y = 9;
    first.velocity = 100;

    actor second;
    second.x = 0;
    second.y = 0;
    second.tile_x = 8;
    second.tile_y = 9;
    second.velocity = 200;

    load_font(renderer, "IMMORTAL.ttf");

    const Uint64 freq = SDL_GetPerformanceFrequency();
    double delta_time = 0;

    Uint64 start = SDL_GetPerformanceCounter();

    // Init socket
    struct sockaddr_in sin;
    int s = init_socket(&sin);

    float base_timerPacket = 0.1f;
    float timer_packet = base_timerPacket;

    // Player id init
    unsigned int player_id = -1;

    // Init struct
    char server_packet[4096];

    // Init players
    actor other_players[100];

    // Current player
    int current_players = 0;

    // Init order packet
    int order_count_player = 0;

    const float timer_timeout_to_get_playerid = 5.;
    float counter_timeout_to_get_playerid = 0;

    do {
        counter_timeout_to_get_playerid -= delta_time;

        if (counter_timeout_to_get_playerid < 0)
        {
            counter_timeout_to_get_playerid = timer_timeout_to_get_playerid;
            send_packet(s, MESSAGE_JOIN, player_id, first.x, first.y, order_count_player, &sin);
            puts("Sending request to join to the server");
        }

        int receive_len = receive_packet(s, server_packet, 4096);
        if (receive_len > 0)
        {
            const maze_packet* packet = (maze_packet*)server_packet;
            const enum MESSAGE_TYPE packet_type = packet->message_type;

            if (packet_type == MESSAGE_WELCOME)
            {
                player_id = packet->player_id;
                send_acknowledge(s, MESSAGE_ACK, time(NULL), packet->message_counter, &sin);
                printf("Your id: %d\n", player_id);
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                SDL_DestroyTexture(texture);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

        SDL_RenderClear(renderer);

        draw_text(renderer, 10, 200, "Loading game...");

        SDL_RenderPresent(renderer);

        Uint64 end = SDL_GetPerformanceCounter();
        delta_time = ((double)end - (double)start) / (double)freq;
        start = end;
    } while (player_id == -1);


    while (window_opened)
    {
        int receive_len = receive_packet(s, server_packet, 4096);

        if (receive_len > 0)
        {
            //printf("Received bytes: %d\n", receive_len);

            const maze_packet* packet = (maze_packet*)server_packet;
            const enum MESSAGE_TYPE packet_type = packet->message_type;

            switch (packet_type)
            {
            case MESSAGE_POSITION:
                //printf("Players: %u %f %f\n", packet->player_id, packet->x, packet->y);
                int checked_player = 0;

                for (size_t i = 0; i < current_players; i++)
                {
                    if (other_players[i].player_id == packet->player_id)
                    {
                        if (other_players[i].last_message_counter < packet->message_counter)
                        {
                            // Delta
                            float delta_x = packet->x - other_players[i].x;
                            float delta_y = packet->y - other_players[i].y;

                            // Delta distance current
                            const float distance = sqrt((delta_x * delta_x) + (delta_y * delta_y));

                            if (distance > 400)
                            {
                                other_players[i].x = packet->x;
                                other_players[i].y = packet->y;
                                other_players[i].last_x = packet->x;
                                other_players[i].last_y = packet->y;
                            }
                            else
                            {
                                other_players[i].last_x = other_players[i].current_x;
                                other_players[i].last_y = other_players[i].current_y;
                            }

                            other_players[i].current_x = packet->x;
                            other_players[i].current_y = packet->y;
                            other_players[i].last_message_counter = packet->message_counter;
                        }

                        checked_player = 1;
                        break;
                    }
                }

                if (!checked_player)
                {
                    other_players[current_players].player_id = packet->player_id;
                    other_players[current_players].last_message_counter = packet->message_counter;
                    other_players[current_players].last_x = packet->x;
                    other_players[current_players].last_y = packet->y;
                    other_players[current_players].current_x = packet->x;
                    other_players[current_players].current_y = packet->y;
                    other_players[current_players].x = packet->x;
                    other_players[current_players].y = packet->y;
                    other_players[current_players].velocity = 100;
                    other_players[current_players].tile_x = 7;
                    other_players[current_players].tile_y = 9;

                    current_players++;
                }

                break;
            case MESSAGE_MELEE:
                printf("%d has attacked!", packet->player_id);
                send_acknowledge(s, MESSAGE_ACK, time(NULL), packet->message_counter, &sin);
                second.x = packet->x;
                second.y = packet->y;
                break;
            case MESSAGE_WELCOME:
            case MESSAGE_JOIN:
            case MESSAGE_ACK:
            default:
                break;
            }


        }

        draw_tile_counter = 0;
        timer_packet -= delta_time;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                window_opened = 0;
            }
            if (event.type == SDL_MOUSEWHEEL)
            {
                scale += event.wheel.y * 0.1f;
                if (scale < 0.1f)
                {
                    scale = 0.1f;
                }
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_k)
                {
                    send_packet(s, MESSAGE_MELEE, player_id, first.x, first.y, order_count_player, &sin);
                }
            }
        }

        const Uint8* keyboard = SDL_GetKeyboardState(NULL);

        if (keyboard[SDL_SCANCODE_UP])
        {
            first.y -= (first.velocity * delta_time);
        }

        if (keyboard[SDL_SCANCODE_DOWN])
        {
            first.y += (first.velocity * delta_time);
        }

        if (keyboard[SDL_SCANCODE_LEFT])
        {
            first.x -= (first.velocity * delta_time);
        }

        if (keyboard[SDL_SCANCODE_RIGHT])
        {
            first.x += (first.velocity * delta_time);
        }
        
        camera_y = first.y - 250;
        camera_x = first.x - 250;

        SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);

        SDL_RenderClear(renderer);

        // SDL_FRect rect  = {0,0,36,36};
        // SDL_RenderCopyF(renderer, font_texture, NULL, &rect);

        // draw_text(renderer, 10,200, "Buon Natale!");

        for (int y = 0; y < 20; y++)
        {
            for (int x = 0; x < 20; x++)
            {
                draw_room(renderer, texture, 16 * 8 * scale * x, 16 * 8 * scale * y, maze[y][x]);
            }
        }

        draw_actor(renderer, texture, &first);
        draw_actor(renderer, texture, &second);

        for (size_t i = 0; i < current_players; i++)
        {
            // Calculate dir
            float delta_x = other_players[i].current_x - other_players[i].last_x;
            float delta_y = other_players[i].current_y - other_players[i].last_y;

            // Calculate vector
            const float magnitude = sqrt((delta_x * delta_x) + (delta_y * delta_y));

            if (magnitude > 0)
            {
                delta_x = delta_x / magnitude;
                delta_y = delta_y / magnitude;

                other_players[i].x += delta_x * other_players[i].velocity * delta_time;
                other_players[i].y += delta_y * other_players[i].velocity * delta_time;

                // printf("Player ID %d \n", other_players[i].player_id);
                // printf("Player x %f \n", other_players[i].x);
                // printf("Player y %f \n", other_players[i].y);
                // printf("Player delta x %f \n", delta_x);
                // printf("Player delta y %f \n", delta_y);
                // printf("Player magnitude %f \n", magnitude);
            }

            draw_actor(renderer, texture, &other_players[i]);
        }

        SDL_RenderPresent(renderer);

        if (timer_packet <= 0)
        {
            // Send packet
            send_packet(s, MESSAGE_POSITION, player_id, first.x, first.y, order_count_player, &sin);

            order_count_player++;

            timer_packet = base_timerPacket;
        }

        Uint64 end = SDL_GetPerformanceCounter();

        delta_time = ((double)end - (double)start) / (double)freq;

        start = end;

        // SDL_Log("Draw tile amount: %d\n", draw_tile_counter);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}