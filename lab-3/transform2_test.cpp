#include "asr.h"

#include <cmath>
#include <ctime>
#include <string>
#include <utility>

static const std::string Vertex_Shader_Source{ R"( // NOLINT(cert-err58-cpp)
    #version 110

    attribute vec4 position;
    attribute vec4 color;
    attribute vec4 texture_coordinates;

    uniform bool texture_enabled;
    uniform mat4 texture_transformation_matrix;

    uniform mat4 model_view_projection_matrix;

    varying vec4 fragment_color;
    varying vec2 fragment_texture_coordinates;

    void main()
    {
        fragment_color = color;
        if (texture_enabled) {
            vec4 transformed_texture_coordinates = texture_transformation_matrix * vec4(texture_coordinates.st, 0.0, 1.0);
            fragment_texture_coordinates = vec2(transformed_texture_coordinates);
        }

        gl_Position = model_view_projection_matrix * position;
        gl_PointSize = 10.0;
    }
)" };

static const std::string Fragment_Shader_Source{ R"( // NOLINT(cert-err58-cpp)
    #version 110

    #define TEXTURING_MODE_ADDITION            0
    #define TEXTURING_MODE_SUBTRACTION         1
    #define TEXTURING_MODE_REVERSE_SUBTRACTION 2
    #define TEXTURING_MODE_MODULATION          3
    #define TEXTURING_MODE_DECALING            4

    uniform bool texture_enabled;
    uniform int texturing_mode;
    uniform sampler2D texture_sampler;

    varying vec4 fragment_color;
    varying vec2 fragment_texture_coordinates;

    void main()
    {
        gl_FragColor = fragment_color;

        if (texture_enabled) {
            if (texturing_mode == TEXTURING_MODE_ADDITION) {
                gl_FragColor += texture2D(texture_sampler, fragment_texture_coordinates);
            } else if (texturing_mode == TEXTURING_MODE_MODULATION) {
                gl_FragColor *= texture2D(texture_sampler, fragment_texture_coordinates);
            } else if (texturing_mode == TEXTURING_MODE_DECALING) {
                vec4 texel_color = texture2D(texture_sampler, fragment_texture_coordinates);
                gl_FragColor.rgb = mix(gl_FragColor.rgb, texel_color.rgb, texel_color.a);
            } else if (texturing_mode == TEXTURING_MODE_SUBTRACTION) {
                gl_FragColor -= texture2D(texture_sampler, fragment_texture_coordinates);
            } else if (texturing_mode == TEXTURING_MODE_REVERSE_SUBTRACTION) {
                gl_FragColor = texture2D(texture_sampler, fragment_texture_coordinates) - gl_FragColor;
            }
        }
    }
)" };

static asr::GeometryPair generate_rectangle_geometry_data(
    asr::GeometryType geometry_type,
    float width, float height,
    unsigned int width_segments_count,
    unsigned int height_segments_count,
    glm::vec4 color = glm::vec4{ 1.0f, 0.3f, 0.3f, 1.0f }
)
{
    assert(
        geometry_type == asr::GeometryType::Triangles
    );

    asr::Vertices vertices;
    asr::Indices indices;

    float half_height{ height * 0.5f };
    float segment_height{ height / static_cast<float>(height_segments_count) };

    float half_width{ width * 0.5f };
    float segment_width{ width / static_cast<float>(width_segments_count) };

    for (auto i = 0U; i <= height_segments_count; ++i) {
        float y{ static_cast<float>(i) * segment_height - half_height };
       
        for (auto j = 0U; j <= width_segments_count; ++j) {
            float x{ static_cast<float>(j) * segment_width - half_width };
            
            vertices.push_back(asr::Vertex{
                x, y, 0.0f,
                color.r, color.g, color.b, color.a
            });
        }
    }

    if (geometry_type == asr::GeometryType::Triangles) {
        for (auto i = 0U; i < height_segments_count; ++i) {
            for (auto j = 0U; j < width_segments_count; ++j) {
                unsigned int index_a{ i * (width_segments_count + 1) + j };
                unsigned int index_b{ index_a + 1 };
                unsigned int index_c{ index_a + (width_segments_count + 1) };
                unsigned int index_d{ index_c + 1 };
             
                indices.insert(indices.end(), { index_a, index_b, index_c });
                indices.insert(indices.end(), { index_b, index_d, index_c });
            }
        }
    }

    return std::make_pair(vertices, indices);
}

static asr::GeometryPair generate_circle_geometry_data(
    asr::GeometryType geometry_type,
    float radius, unsigned int segment_count,
    glm::vec4 color = glm::vec4{ 1.0f, 0.3f, 0.3f, 1.0f }
)
{
    assert(
        geometry_type == asr::GeometryType::Triangles
    );

    asr::Vertices vertices;
    asr::Indices indices;

    vertices.push_back(asr::Vertex{
        0.0f, 0.0f, 0.0f,
        color.r, color.g, color.b, color.a
    });
    
    float angle{ 0.0f };
    float angle_delta{ asr::two_pi / static_cast<float>(segment_count) };

    float x{ std::cosf(angle) * radius };
    float y{ std::sinf(angle) * radius };
 
    vertices.push_back(asr::Vertex{
        x, y, 0.0f,
        color.r, color.g, color.b, color.a,
    });

    for (auto i = 0U; i < segment_count; ++i) {
        if (geometry_type == asr::GeometryType::Triangles) {
            indices.push_back(0);
            indices.push_back(vertices.size() - 1);
        }
        float next_x{ std::cosf(angle + angle_delta) * radius };
        float next_y{ std::sinf(angle + angle_delta) * radius };

        vertices.push_back(asr::Vertex{
            next_x, next_y, 0.0f,
            color.r, color.g, color.b, color.a
        });
        indices.push_back(vertices.size() - 1);

        angle += angle_delta;
    }

    return std::make_pair(vertices, indices);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    using namespace asr;

    create_window(500, 500, "Transform Test 2 on ASR Version 1.2");
    create_shader(Vertex_Shader_Source, Fragment_Shader_Source);

    unsigned int hour{ 0 };
    unsigned int minutes{ 0 };
    unsigned int seconds{ 0 };

    float hoursDegrees{ 0.0f };
    float minutesDegrees{ 0.0f };
    float secondsDegrees{ 0.0f };

    float radius{ 0.3f };
    unsigned int circle_segments{ 30U };

    glm::vec4 circle_min_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    auto [triangle_vertices_minutes, triangle_indices_minutes] = generate_circle_geometry_data(Triangles, radius, circle_segments, circle_min_color);
    auto trianglesCircleMinutes = create_geometry(Triangles, triangle_vertices_minutes, triangle_indices_minutes);

    glm::vec4 circle_centre_color{ 1.0f, 0.3f, 0.3f, 1.0f };
    auto [triangle_vertices_centre, triangle_indices_centre] = generate_circle_geometry_data(Triangles, radius, circle_segments, circle_centre_color);
    auto trianglesCircleCentre = create_geometry(Triangles, triangle_vertices_centre, triangle_indices_centre);

    float width{ 0.09f }, height{ 0.09f };
    unsigned int width_segments{ 1U }, height_segments{ 1U };

    glm::vec4 rect_color_one{ 1.0f, 0.0f, 0.0f, 1.0f };
    auto [triangle_vertices_rect_one, triangle_indices_rect_one] = generate_rectangle_geometry_data(Triangles, width, height, width_segments, height_segments,rect_color_one);
    auto trianglesRectOne = create_geometry(Triangles, triangle_vertices_rect_one, triangle_indices_rect_one);

    glm::vec4 rect_color_two{ 1.0f, 0.3f, 0.3f, 1.0f };
    auto [triangle_vertices_rect_two, triangle_indices_rect_two] = generate_rectangle_geometry_data(Triangles, width, height, width_segments, height_segments, rect_color_two);
    auto trianglesRectTwo = create_geometry(Triangles, triangle_vertices_rect_two, triangle_indices_rect_two);


    prepare_for_rendering();

    bool should_stop{ false };

    time_t currentTime;
    struct tm* localTime;

    while (!should_stop) 
    {
        time(&currentTime);                   
        localTime = localtime(&currentTime); 

        hour = localTime->tm_hour;
        minutes = localTime->tm_min;
        seconds = localTime->tm_sec;

        secondsDegrees = ((seconds / 60.0f) * 360.0f);
        minutesDegrees = ((minutes / 60.0f) * 360.0f) + ((seconds / 60.0f) * 6);
        hoursDegrees = ((hour / 12.0f) * 360) + ((minutes / 60.0f) * 30);

        process_window_events(&should_stop);
        prepare_to_render_frame();
        set_matrix_mode(MatrixMode::Model);

        push_matrix();
        {
            rotate_matrix(glm::vec3{ 0.0f, 0.0f, -hoursDegrees / 57.3f });
            translate_matrix(glm::vec3{ 0.0f, 0.25f, 0.0f });
            scale_matrix(glm::vec3{ 0.3f, 6.0f, 0.0f });
            set_geometry_current(&trianglesRectOne);
            render_current_geometry();
        }
        pop_matrix();
        push_matrix();
        {
            rotate_matrix(glm::vec3{ 0.0f, 0.0f, -minutesDegrees / 57.3f }); 
            translate_matrix(glm::vec3{ 0.0f, 0.30f, 0.0f });
            scale_matrix(glm::vec3{ 0.2f, 7.0f, 0.0f });
            set_geometry_current(&trianglesRectOne);
            render_current_geometry();
        }
        pop_matrix();
        push_matrix();
        {
            rotate_matrix(glm::vec3{ 0.0f, 0.0f, -secondsDegrees / 57.3f });
            translate_matrix(glm::vec3{ 0.0f, 0.35f, 0.0f });
            scale_matrix(glm::vec3{ 0.1f, 8.0f, 0.0f });
            set_geometry_current(&trianglesRectOne);
            render_current_geometry();
        }
        pop_matrix();
        push_matrix();
        {
            scale_matrix(glm::vec3{ 0.12f });
            set_geometry_current(&trianglesCircleCentre);
            render_current_geometry();
        }
        pop_matrix();

        for (float angle = 0.0; angle <= two_pi; angle += pi / 30.0f) {
            push_matrix();
            {
                rotate_matrix(glm::vec3{ 0.0f, 0.0f, angle });
                translate_matrix(glm::vec3{ 0.82f, 0.0f, 0.0f });
                scale_matrix(glm::vec3{ 0.04f });
                set_geometry_current(&trianglesCircleMinutes);
                render_current_geometry();
            }
            pop_matrix();
        }
        for (float angle = 0.0; angle <= two_pi; angle += half_pi) {
            push_matrix();
            {
                rotate_matrix(glm::vec3{ 0.0f, 0.0f, angle });
                translate_matrix(glm::vec3{ 0.84f, 0.0f, 0.0f });
                rotate_matrix(glm::vec3{ 0.0f, 0.0f, pi / 4.0f });
                scale_matrix(glm::vec3{ 1.0f });
                set_geometry_current(&trianglesRectOne);
                render_current_geometry();
            }
            pop_matrix();
        }
        for (float angle = 0.0; angle <= two_pi; angle += pi/6.0f) {
            push_matrix();
            {
                rotate_matrix(glm::vec3{ 0.0f, 0.0f, angle });
                translate_matrix(glm::vec3{ 0.86f, 0.0f, 0.0f });
                scale_matrix(glm::vec3{ 0.35f });
                set_geometry_current(&trianglesRectTwo);
                render_current_geometry();
            }
            pop_matrix();
        }

        finish_frame_rendering();
    }
    destroy_geometry(trianglesRectOne);
    destroy_geometry(trianglesRectTwo);
    destroy_geometry(trianglesCircleMinutes);
    destroy_geometry(trianglesCircleCentre);
    destroy_shader();
    destroy_window();
    return 0;
}