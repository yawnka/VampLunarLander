/**
* Author: Yanka Sikder
* Assignment: Lunar Lander
* Date due: 2024-10-26, 11:59pm ( EXTENDED DUE TO CONFERENCE (SLS EXCUSED) & FILE ISSUES)
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_WIDTH 20
#define LEVEL1_HEIGHT 7
// defining constants based on the tilset
#define TILE_COUNT_X 4  // 4 tiles horizontally
#define TILE_COUNT_Y 1  // 1 tile vertically

#ifdef _WINDOWS
    #include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "Entity.h"
#include <vector>
#include <ctime>
#include "cmath"
#include "Map.h"
#include <string>

// ————— CONSTANTS ————— //
constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 700;

constexpr float BG_RED     = 0.2,
                BG_GREEN   = 0.2,
                BG_BLUE    = 0.2,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
              VIEWPORT_Y = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr glm::vec3 PLAYER_IDLE_LOCATION = glm::vec3(3.0f, 2.0f, 0.0f);
constexpr glm::vec3 INIT_FINAL_SCREEN_SCALE = glm::vec3(4.0f, 4.0f, 1.0f);

constexpr char MAP_TILESET_FILEPATH[] = "tileset.png",
ACCOMPLISHED_FILEPATH[] = "missionComp.png",
FAILED_FILEPATH[] = "missionFail.png";

constexpr char FONTSHEET_FILEPATH[]   = "font1.png";
constexpr int FONTBANK_SIZE = 16;

// ————— STRUCTS AND ENUMS —————//
enum AppStatus  { RUNNING, TERMINATED };
enum FilterType { NEAREST, LINEAR     };

struct GameState { Entity* player; Map* map; };

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix, g_accomplished_matrix, g_failed_matrix;

GLuint g_accomplished_texture_id, g_failed_texture_id;

GLuint g_font_texture_id;

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

unsigned int LEVEL_1_DATA[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 3, 0, 0, 0, 0,
    0, 0, 0, 2, 1, 1, 3, 0, 1, 1, 0, 3, 2, 1, 2, 1, 1, 1, 0, 3,
    2, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

float g_previous_ticks = 0.0f,
      g_accumulator    = 0.0f;

bool game_over;

void draw_text(ShaderProgram *shader_program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position);

void initialise();
void process_input();
void update();
void render();
void shutdown();

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return texture_id;
}

// taken from lecture: sprites-and-text to draw UI for fuel (Extra - Credit)
void draw_text(ShaderProgram *shader_program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander Vamp",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -5.0f, 2.5f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // MAP SETUP //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH);
    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, TILE_COUNT_X, TILE_COUNT_Y);

    
    // ————— VAMPIRE ————— //
    std::vector<GLuint> vampire_textures_ids = {
        load_texture("vamp.png"),   // IDLE spritesheet
        load_texture("bat.png")  // IDLE spritesheet
    };

    std::vector<std::vector<int>> vampire_animations = {
        {0},       // IDLE animation frames
        {0}  // ATTACK animation frames
    };
    
    // font texture
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH);
    
    g_game_state.player = new Entity(
        vampire_textures_ids,  // a list of texture IDs
        1.0f,                // translation speed; irrelevant in this problem
        vampire_animations,    // list of animation frames for each type of animation
        0.0f,                // animation time
        1,                   // number of frames for idle animation
        0,                   // current frame index
        1,                   // current animation col amount
        1,                   // current animation row amount
        IDLE                 // current animation
    );

    g_game_state.player->set_position(PLAYER_IDLE_LOCATION);
    
    g_accomplished_matrix = glm::mat4(1.0f);
    g_accomplished_matrix = glm::scale(g_accomplished_matrix, INIT_FINAL_SCREEN_SCALE);
    
    g_failed_matrix = glm::mat4(1.0f);
    g_failed_matrix = glm::scale(g_failed_matrix, INIT_FINAL_SCREEN_SCALE);
    
    g_accomplished_texture_id  = load_texture(ACCOMPLISHED_FILEPATH);
    g_failed_texture_id  = load_texture(FAILED_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void process_input()
{
    g_game_state.player->set_animation_state(IDLE);
    g_game_state.player->set_acceleration(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    // Handle rotation
    if (key_state[SDL_SCANCODE_D]) {
        g_game_state.player->set_rotation(90.0f); // Rotate to 90 degrees
    }
    else if (key_state[SDL_SCANCODE_A]) {
        g_game_state.player->set_rotation(-90.0f); // Rotate to -90 degrees
    }
    else if (key_state[SDL_SCANCODE_W]) {
        g_game_state.player->set_rotation(0.0f); // Rotate back to 0 degrees
    }

    // Accelerate only if there is fuel
    if (g_game_state.player->has_fuel() && key_state[SDL_SCANCODE_SPACE]) {
        g_game_state.player->set_animation_state(ATTACK);
        g_game_state.player->decrease_fuel(Entity::FUEL_CONSUMPTION_RATE);
    }
}


void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, NULL, 0, g_game_state.map);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;

    // Y-position threshold for falling off the screen as the lander did not land
    float fall_threshold = -5.5f;

    // Checking if the player has fallen below the threshold
    if (g_game_state.player->get_position().y < fall_threshold)
    {
        g_game_state.player->set_game_status(true); // End the game if so
        g_game_state.player->set_collided_tile(2);  // Setting tile to 2 to trigger "Mission Failed"
    }
    
    g_view_matrix = glm::mat4(1.0f);
    
    // Camera follows the player as long as the game is not over
    if (!g_game_state.player->get_game_status()) {
        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, 0.0f, 0.0f));
    }
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coords[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coords);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    game_over = g_game_state.player->get_game_status();
    if (!game_over) {
        // Setting view matrix to follow the player
        g_shader_program.set_view_matrix(g_view_matrix);
        g_game_state.player->render(&g_shader_program);
        g_game_state.map->render(&g_shader_program);

        // Resetting the view matrix for fuel UI to make them fixed on the screen as the player moves
        glm::mat4 ui_view_matrix = glm::mat4(1.0f);
        g_shader_program.set_view_matrix(ui_view_matrix);

        // drawing the word using the font png to write the fuel remaining, middle top location:
        std::string fuel_text = "Fuel: " + std::to_string(static_cast<int>(g_game_state.player->get_fuel())) + "%";
        draw_text(&g_shader_program, g_font_texture_id, fuel_text, 0.5f, 0.05f, glm::vec3(-2.0f, 2.0f, 0.0f));

    } else {
        g_view_matrix = glm::mat4(1.0f);
        g_shader_program.set_view_matrix(g_view_matrix);
        
        if (g_game_state.player->get_collided_tile() == 3) {
            draw_object(g_accomplished_matrix, g_accomplished_texture_id);  // Mission Accomplished Screen
        } else {
            draw_object(g_failed_matrix, g_failed_texture_id);  // Mission Failed Screen
        }
    }
    SDL_GL_SwapWindow(g_display_window);
}



void shutdown()
{
    SDL_Quit();
    delete   g_game_state.player;
    delete   g_game_state.map;
}


int main(int argc, char* argv[])
{
    std::cout << "Kinda buggy in identifying tiles to show end screen" << std::endl;
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}



