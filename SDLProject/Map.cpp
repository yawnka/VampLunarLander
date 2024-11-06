#include "Map.h"

#define TILE_COUNT_X 4  // 4 tiles horizontally
#define TILE_COUNT_Y 1  // 1 tile vertically

// Constructor
Map::Map(int width, int height, unsigned int *level_data, GLuint texture_id, float tile_size, int tile_count_x, int tile_count_y)
    : m_width(width), m_height(height),
      m_level_data(level_data), m_texture_id(texture_id),
      m_tile_size(tile_size), m_tile_count_x(tile_count_x),
      m_tile_count_y(tile_count_y)
{
    build();
}

// Build function to initialize vertices and texture coordinates for each tile
void Map::build()
{
    float padding = 0.0f;  // Set padding to zero to remove gaps within tiles

    for (int y_coord = 0; y_coord < m_height; y_coord++)
    {
        for (int x_coord = 0; x_coord < m_width; x_coord++)
        {
            int tile = m_level_data[y_coord * m_width + x_coord];
            
            // Skip if it's an empty tile (0)
            if (tile == 0) continue;

            // Calculate UV coordinates based on tile index
            float tile_width = 1.0f / TILE_COUNT_X;
            float tile_height = 1.0f / TILE_COUNT_Y;
            
            float u_coord = (tile % TILE_COUNT_X) * tile_width;
            float v_coord = (tile / TILE_COUNT_X) * tile_height;

            float x_pos = m_tile_size * x_coord;
            float y_pos = -m_tile_size * y_coord;

            // Store vertex positions
            m_vertices.insert(m_vertices.end(), {
                x_pos, y_pos,
                x_pos, y_pos - m_tile_size,
                x_pos + m_tile_size, y_pos - m_tile_size,
                x_pos, y_pos,
                x_pos + m_tile_size, y_pos - m_tile_size,
                x_pos + m_tile_size, y_pos
            });

            // Store texture coordinates
            m_texture_coordinates.insert(m_texture_coordinates.end(), {
                u_coord, v_coord,
                u_coord, v_coord + tile_height,
                u_coord + tile_width, v_coord + tile_height,
                u_coord, v_coord,
                u_coord + tile_width, v_coord + tile_height,
                u_coord + tile_width, v_coord
            });
        }
    }

    // Set map boundaries
    m_left_bound = 0;
    m_right_bound = m_tile_size * m_width;
    m_top_bound = 0;
    m_bottom_bound = -m_tile_size * m_height;
}



// Render function
void Map::render(ShaderProgram *program)
{
    glm::mat4 model_matrix = glm::mat4(1.0f);
    program->set_model_matrix(model_matrix);
    
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, m_vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, m_texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    
    // Draw all the vertices
    glDrawArrays(GL_TRIANGLES, 0, (int) m_vertices.size() / 2);
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

// Check if a tile is solid for collision detection
bool Map::is_solid(glm::vec3 position, float *penetration_x, float *penetration_y)
{
    *penetration_x = 0;
    *penetration_y = 0;

    // Calculate tile indices for the position
    int tile_x = static_cast<int>(floor(position.x / m_tile_size));
    int tile_y = static_cast<int>(-ceil(position.y / m_tile_size));

    // Check bounds of the map
    if (tile_x < 0 || tile_x >= m_width || tile_y < 0 || tile_y >= m_height) return false;

    // Get tile type from level data
    int tile = get_tile_type(tile_x, tile_y);
    if (tile == 0) return false; // Not solid

    // Calculate the center of the tile
    float tile_center_x = tile_x * m_tile_size + m_tile_size / 2;
    float tile_center_y = -tile_y * m_tile_size - m_tile_size / 2;

    // Calculate penetration distances
    *penetration_x = (m_tile_size / 2) - fabs(position.x - tile_center_x);
    *penetration_y = (m_tile_size / 2) - fabs(position.y - tile_center_y);

    return true;
}


// Function to get tile type at specific coordinates
int Map::get_tile_type(int x, int y) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return -1;  // Out of bounds
    }
    return m_level_data[y * m_width + x];
}
