#include "asr.h"

#include <string>
#include <utility>

static const std::string Vertex_Shader_Source{ R"( // NOLINT(cert-err58-cpp)
    #version 110

    attribute vec4 position;
    attribute vec4 color;

    uniform mat4 model_view_projection_matrix;

    varying vec4 fragment_color;

    void main()
    {
        fragment_color = color;

        gl_Position = model_view_projection_matrix * position;
        gl_PointSize = 10.0;
    }
)" };

static const std::string Fragment_Shader_Source{ R"( // NOLINT(cert-err58-cpp)
    #version 110

    varying vec4 fragment_color;

    void main()
    {
        gl_FragColor = fragment_color;
    }
)" };

static asr::GeometryPair generate_sphere_geometry_data(
    asr::GeometryType geometryType,
    float radius,
    unsigned int width_segments_count,
    unsigned int height_segments_count,
    glm::vec4 color = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }
)
{
    assert(
        geometryType == asr::GeometryType::Triangles ||
        geometryType == asr::GeometryType::Lines ||
        geometryType == asr::GeometryType::Points
    );

    asr::Vertices vertices;
    asr::Indices indices;

    for (auto i = 0U; i <= height_segments_count; ++i) {
        float v { static_cast<float>(i) / static_cast<float>(height_segments_count) };

        float phi {v * asr::pi};
            
        for (auto j = 0U; j <= width_segments_count; ++j) {
            float u{ static_cast<float>(j) / static_cast<float>(width_segments_count) };

            float theta{ u * asr::two_pi };

            float cos_phi{ std::cosf(phi) };
            float sin_phi{ std::sinf(phi) };
            float cos_theta{ std::cosf(theta) };
            float sin_theta{ std::sinf(theta) };

            float x{ radius * cos_theta * sin_phi };
            float y{ radius * cos_phi };
            float z{ radius * sin_phi * sin_theta };

            vertices.push_back(asr::Vertex { x, y, z, color.r, color.g, color.b, color.a });

            if (geometryType == asr::GeometryType::Points)
            {
                indices.push_back(vertices.size() - 1);
            }
        }
    }

    if (geometryType == asr::GeometryType::Lines || geometryType == asr::GeometryType::Triangles) {
        for (auto rows = 0U; rows < height_segments_count; ++rows) {
            for (auto columns = 0U; columns < width_segments_count; ++columns) {
                unsigned int index_a{ rows * (width_segments_count + 1) + columns };
                unsigned int index_b{ index_a + 1 };
                unsigned int index_c{ index_a + (width_segments_count + 1) };
                unsigned int index_d{ index_c + 1 };

                if (geometryType == asr::GeometryType::Lines) {
                    indices.insert(indices.end(), { index_a, index_b, index_b, index_c, index_c, index_a });
                    indices.insert(indices.end(), {index_b, index_d, index_d, index_c, index_c, index_b });
                } else {
                    if (rows != 0) {
                        indices.insert(indices.end(), { index_a, index_b, index_c });
                    } if (rows != height_segments_count-1)
                    {
                        indices.insert(indices.end(), { index_b, index_d, index_c });
                    }
                }
            }
        }
    }

    return std::make_pair(vertices, indices);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    using namespace asr;

    create_window(500, 500, "Sphere Test on ASR Version 1.1");
    create_shader(Vertex_Shader_Source, Fragment_Shader_Source);

    float radius{0.5f};
    unsigned int width_segments{ 20 }, height_segments{ 20 };

    auto [triangle_vertices, triangle_indices] =
        generate_sphere_geometry_data(
            Triangles, radius, width_segments, height_segments
        );
    auto triangles =
        create_geometry(
            Triangles, triangle_vertices, triangle_indices
        );

    glm::vec4 edge_color{ 1.0f, 0.7f, 0.7f, 1.0f };
    auto [edge_vertices, edge_indices] =
        generate_sphere_geometry_data(
            Lines, radius * 1.005f, width_segments, height_segments, edge_color
        );
    auto lines = create_geometry(Lines, edge_vertices, edge_indices);

    glm::vec4 vertex_color{ 1.0f, 0.0f, 0.0f, 1.0f };
    auto [vertices, vertex_indices] =
        generate_sphere_geometry_data(
            Points, radius * 1.01f, width_segments, height_segments, vertex_color
        );
    auto points = create_geometry(Points, vertices, vertex_indices);

    glm::vec3 sphere_position{0.0f, 0.0f, 0.0f};
    glm::vec3 sphere_rotation{0.0f, 0.01f, 0.0f};
    glm::vec3 sphere_scale{1.0f, 1.0f, 1.0f};


    prepare_for_rendering();
    enable_face_culling();
    enable_depth_test();
    set_line_width(3);

    static const float CAMERA_SPEED{ 0.1f };
    static const float CAMERA_ROT_SPEED{ 0.01f };
    static const float CAMERA_NEAR_PLANE{ 0.01f };
    static const float CAMERA_FAR_PLANE{ 100.0f };
    static const float CAMERA_FOV{ 1.13f };
    glm::vec3 camera_position{ 0.0f, 0.75f, 1.1f };
    glm::vec3 camera_rotation{ -0.6f, 0.0f, 0.0f }; 

    set_keys_down_event_handler([&](const uint8_t* keys) {
        if (keys[SDL_SCANCODE_W]) {
            camera_rotation.x += CAMERA_ROT_SPEED;
        }
        if (keys[SDL_SCANCODE_S]) {
            camera_rotation.x -= CAMERA_ROT_SPEED;
        }
        if (keys[SDL_SCANCODE_A]) {
            camera_rotation.y += CAMERA_ROT_SPEED;
        }
        if (keys[SDL_SCANCODE_D]) {
            camera_rotation.y -= CAMERA_ROT_SPEED;
        }
        if (keys[SDL_SCANCODE_UP]) {
            camera_position -= glm::vec3{ get_view_matrix() * glm::vec4{ 0.0f,0.0f,1.0f,0.0f } *CAMERA_SPEED };
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            camera_position += glm::vec3{ get_view_matrix() * glm::vec4{ 0.0f,0.0f,1.0f,0.0f } *CAMERA_SPEED };
        }
    });

    set_matrix_mode(MatrixMode::Projection);
    load_perspective_projection_matrix(CAMERA_FOV, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);

    bool should_stop{ false };

    while (!should_stop) {
        process_window_events(&should_stop);

        prepare_to_render_frame();

        set_matrix_mode(MatrixMode::View);
        load_identity_matrix();
        translate_matrix(camera_position);
        rotate_matrix(camera_rotation);

        set_matrix_mode(MatrixMode::Model);
        load_identity_matrix();
        translate_matrix(sphere_position);
        rotate_matrix(sphere_rotation);
        scale_matrix(sphere_scale);

        set_geometry_current(&triangles);
        render_current_geometry();

        set_geometry_current(&lines);
        render_current_geometry();

        set_geometry_current(&points);
        render_current_geometry();

        finish_frame_rendering();
    }

    destroy_geometry(triangles);
    destroy_geometry(lines);
    destroy_geometry(points);

    destroy_shader();
    destroy_window();

    return 0;
}
