/**
* Author: Yanka Sikder
* Assignment: Lunar Lander
* Date due: 2024-10-26, 11:59pm ( EXTENDED DUE TO CONFERENCE (SLS EXCUSED) & FILE ISSUES)
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"
#include <vector>


// Default constructor
Entity::Entity()
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
      m_speed(0.0f), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
      m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
      m_current_animation(IDLE)
{
}

// Parameterized constructor
Entity::Entity(std::vector<GLuint> texture_ids, float speed,
               std::vector<std::vector<int>> animations, float animation_time,
               int animation_frames, int animation_index, int animation_cols,
               int animation_rows, Animation animation)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
      m_speed(speed), m_texture_ids(texture_ids), m_animations(animations),
      m_animation_cols(animation_cols), m_animation_frames(animation_frames),
      m_animation_index(animation_index), m_animation_rows(animation_rows),
      m_animation_time(animation_time), m_current_animation(animation)
{
    set_animation_state(m_current_animation);  // Initialize animation state
}

Entity::~Entity() { }

void Entity::set_animation_state(Animation new_animation)
{
    m_current_animation = new_animation;

    // Update the texture and animation indices based on the current animation
    m_animation_indices = m_animations[m_current_animation].data();
    m_animation_rows = m_animations[m_current_animation].size();
}

// Render the appropriate texture and animation frame
void Entity::draw_sprite_from_texture_atlas(ShaderProgram* program)
{
    GLuint current_texture = m_texture_ids[m_current_animation];  // Get the right texture

    float u_coord = (float) (m_animation_index % m_animation_cols) / (float) m_animation_cols;
    float v_coord = (float) (m_animation_index / m_animation_cols) / (float) m_animation_rows;

    float width = 1.0f / (float) m_animation_cols;
    float height = 1.0f / (float) m_animation_rows;

    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width,
        v_coord, u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    glBindTexture(GL_TEXTURE_2D, current_texture);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
                          tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity* other) const
{
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

    return x_distance < 0.0f && y_distance < 0.0f;
}

void const Entity::check_collision_y(Entity *collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity))
        {
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));
            if (m_velocity.y > 0)
            {
                m_position.y   -= y_overlap;
                m_velocity.y    = 0;

                // Collision!
                m_collided_top  = true;
            } else if (m_velocity.y < 0)
            {
                m_position.y      += y_overlap;
                m_velocity.y       = 0;

                // Collision!
                m_collided_bottom  = true;
            }
        }
    }
}

void const Entity::check_collision_x(Entity *collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity))
        {
            float x_distance = fabs(m_position.x - collidable_entity->m_position.x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->m_width / 2.0f));
            if (m_velocity.x > 0)
            {
                m_position.x     -= x_overlap;
                m_velocity.x      = 0;

                // Collision!
                m_collided_right  = true;
                
            } else if (m_velocity.x < 0)
            {
                m_position.x    += x_overlap;
                m_velocity.x     = 0;
 
                // Collision!
                m_collided_left  = true;
            }
        }
    }
}


bool Entity::check_collision_y(Map *map)
{
    glm::vec3 top = glm::vec3(m_position.x, m_position.y + (m_height / 2), m_position.z);
    glm::vec3 bottom = glm::vec3(m_position.x, m_position.y - (m_height / 2), m_position.z);

    float penetration_x = 0, penetration_y = 0;

    int tileType = -1; // Initialize tileType

    // Check collisions below
    if (map->is_solid(bottom, &penetration_x, &penetration_y) && m_velocity.y < 0)
    {
        m_position.y += penetration_y;
        m_velocity.y = 0;
        m_collided_bottom = true;

        // Get the tile type at the bottom collision position
        int tile_x = static_cast<int>(floor(bottom.x / map->get_tile_size()));
        int tile_y = static_cast<int>(-ceil(bottom.y / map->get_tile_size()));
        tileType = map->get_tile_type(tile_x, tile_y);
    }

    // Check collisions above
    if (map->is_solid(top, &penetration_x, &penetration_y) && m_velocity.y > 0)
    {
        m_position.y -= penetration_y;
        m_velocity.y = 0;
        m_collided_top = true;

        // Get the tile type at the top collision position
        int tile_x = static_cast<int>(floor(top.x / map->get_tile_size()));
        int tile_y = static_cast<int>(-ceil(top.y / map->get_tile_size()));
        tileType = map->get_tile_type(tile_x, tile_y);
    }

    // Set game status based on tile type
    if (tileType == 3) {
        set_game_status(true);
        set_collided_tile(3);  // Mission Accomplished
    } else if (tileType > 0) {
        set_game_status(true);
        set_collided_tile(tileType);  // Mission Failed
    }

    return (m_collided_top || m_collided_bottom);
}

void Entity::check_collision_x(Map *map)
{
    glm::vec3 left = glm::vec3(m_position.x - (m_width / 2), m_position.y, m_position.z);
    glm::vec3 right = glm::vec3(m_position.x + (m_width / 2), m_position.y, m_position.z);

    float penetration_x = 0, penetration_y = 0;

    int tileType = -1; // Initializing the tileType

    // Check collision on the left
    if (map->is_solid(left, &penetration_x, &penetration_y) && m_velocity.x < 0)
    {
        m_position.x += penetration_x;
        m_velocity.x = 0;
        m_collided_left = true;

        // Get the tile type at the left collision position
        int tile_x = static_cast<int>(floor(left.x / map->get_tile_size()));
        int tile_y = static_cast<int>(-ceil(left.y / map->get_tile_size()));
        tileType = map->get_tile_type(tile_x, tile_y);
    }

    // Check collision on the right
    if (map->is_solid(right, &penetration_x, &penetration_y) && m_velocity.x > 0)
    {
        m_position.x -= penetration_x;
        m_velocity.x = 0;
        m_collided_right = true;

        // Get the tile type at the right collision position
        int tile_x = static_cast<int>(floor(right.x / map->get_tile_size()));
        int tile_y = static_cast<int>(-ceil(right.y / map->get_tile_size()));
        tileType = map->get_tile_type(tile_x, tile_y);
    }

    // Set game status based on tile type
    if (tileType == 3) {
        set_game_status(true);
        set_collided_tile(3);  // Mission Accomplished Tile
    } else if (tileType > 0) {
        set_game_status(true);
        set_collided_tile(tileType);  // Mission Failed Tiles which are every other tile
    }
}



void Entity::update(float delta_time, Entity *player, Entity *collidable_entities, int collidable_entity_count, Map *map)
{
    if (!m_is_active) return;

    m_collided_top    = false;
    m_collided_bottom = false;
    m_collided_left   = false;
    m_collided_right  = false;

    if (m_animation_indices != NULL)
    {
        if (glm::length(m_movement) != 0)
        {
            m_animation_time += delta_time;
            float frames_per_second = (float) 1 / SECONDS_PER_FRAME;

            if (m_animation_time >= frames_per_second)
            {
                m_animation_time = 0.0f;
                m_animation_index++;

                if (m_animation_index >= m_animation_frames)
                {
                    m_animation_index = 0;
                }
            }
        }
    }

    m_position.y += m_velocity.y * delta_time;

    if (!m_collided_top) {
        m_velocity.y += GRAVITY * delta_time;
    }

    glm::vec3 acceleration(0.0f, 0.0f, 0.0f);
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    // Apply acceleration if there is fuel and the player presses the space key
    if (has_fuel() && key_state[SDL_SCANCODE_SPACE]) {
        if (m_rotation == 0.0f) {
            acceleration.y = ACCELERATION;
        } else if (m_rotation == 90.0f) {
            acceleration.x = ACCELERATION;
        } else if (m_rotation == -90.0f) {
            acceleration.x = -ACCELERATION;
        }
    }

    m_velocity.x += acceleration.x * delta_time;

    // Apply drift
    if (m_velocity.x > 0) {
        m_velocity.x -= drift * delta_time;
        if (m_velocity.x < 0) m_velocity.x = 0;
    } else if (m_velocity.x < 0) {
        m_velocity.x += drift * delta_time;
        if (m_velocity.x > 0) m_velocity.x = 0;
    }

    m_position.x += m_velocity.x * delta_time;
    m_position.y += acceleration.y * delta_time;

    check_collision_x(collidable_entities, collidable_entity_count);
    check_collision_x(map);
    check_collision_y(collidable_entities, collidable_entity_count);
    bool collision_y = check_collision_y(map);

    if (collision_y) {
        set_game_status(true);
    }

    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::rotate(m_model_matrix, glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, -1.0f));
}




void Entity::render(ShaderProgram* program)
{
    program->set_model_matrix(m_model_matrix);

    if (m_animation_indices != nullptr) draw_sprite_from_texture_atlas(program);
    
    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

//    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

