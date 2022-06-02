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

static asr::GeometryPair generate_box_geometry_data(
    asr::GeometryType geometryType,
    float width, float height, float depth,
    unsigned int width_segments_count,
    unsigned int height_segments_count,
    unsigned int depth_segments_count,
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

    float half_height{ height * 0.5f };
    float segment_height{ height / static_cast<float>(height_segments_count) };

    float half_width{ width * 0.5f };
    float segment_width{ width / static_cast<float>(width_segments_count) };

    float half_depth{ depth * 0.5f };
    float segment_depth{ depth / static_cast<float>(depth_segments_count) };

    unsigned int offset;

    for (unsigned int side = 0; side < 6; ++side) {
        offset = vertices.size();

        for (auto i = 0U; i <= height_segments_count; ++i) {
            float y{ static_cast<float>(i) * segment_height - half_height };
            float z{ static_cast<float>(i) * segment_depth - half_height };

            for (auto j = 0U; j <= width_segments_count; ++j) {
                float x{ static_cast<float>(j) * segment_width - half_width };

                if (side == 0) {
                    vertices.push_back(asr::Vertex{x, y, half_depth,color.r, color.g, color.b, color.a });
                } else if (side == 1) {
                    float z{ static_cast<float>(j) * segment_depth - half_height };
                    vertices.push_back(asr::Vertex{half_width, y, z,color.r, color.g, color.b, color.a});
                } else if (side == 2) {
                    float z{ static_cast<float>(j) * segment_width - half_width };
                    vertices.push_back(asr::Vertex{-half_width, y, z,color.r, color.g, color.b, color.a});
                } else if (side == 3) {
                    vertices.push_back(asr::Vertex{x, y, -half_depth,color.r, color.g, color.b, color.a});
                } else if (side == 4) {
                    vertices.push_back(asr::Vertex{x, -half_height, z,color.r, color.g, color.b, color.a});
                } else if (side == 5) {
                    vertices.push_back(asr::Vertex{x, half_height, z,color.r, color.g, color.b, color.a});
                }

                if (geometryType == asr::GeometryType::Points) {
                    indices.push_back(vertices.size() - 1);
                }
            }
        }

        if (geometryType == asr::GeometryType::Lines || geometryType == asr::GeometryType::Triangles) {
            for (auto i = 0U; i < height_segments_count; ++i) {
                for (auto j = 0U; j < width_segments_count; ++j) {
                    unsigned int index_a{ i * (width_segments_count + 1) + j };
                    unsigned int index_b{ index_a + 1 };
                    unsigned int index_c{ index_a + (width_segments_count + 1) };
                    unsigned int index_d{ index_c + 1 };

                    index_a += offset; index_b += offset;
                    index_c += offset; index_d += offset;

                    if (geometryType == asr::GeometryType::Lines) {
                        indices.insert(indices.end(), { index_a, index_b, index_b, index_c, index_c, index_a });
                        indices.insert(indices.end(), { index_b, index_d, index_d, index_c, index_c, index_b });
                    } else if (side & 1) {
                        indices.insert(indices.end(), { index_c, index_b, index_a });
                        indices.insert(indices.end(), { index_c, index_d, index_b });
                    } else {
                        indices.insert(indices.end(), { index_a, index_b, index_c });
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

    create_window(500, 500, "Box Test on ASR Version 1.1");
    create_shader(Vertex_Shader_Source, Fragment_Shader_Source);

    float width{ 1.0f }, height{ 1.0f }, depth{ 1.0f };
    unsigned int width_segments{ 5 }, height_segments{ 5 }, depth_segments{ 5 };;

    auto [triangle_vertices, triangle_indices] =
        generate_box_geometry_data(
            Triangles, width, height, depth, width_segments, height_segments, depth_segments
        );
    auto triangles =
        create_geometry(
            Triangles, triangle_vertices, triangle_indices
        );

    glm::vec4 edge_color{ 1.0f, 0.7f, 0.7f, 1.0f };
    auto [edge_vertices, edge_indices] =
        generate_box_geometry_data(
            Lines, width * 1.003f, height * 1.003f, depth * 1.003f, width_segments, height_segments, depth_segments, edge_color
        );
    for (auto& vertex : edge_vertices) { vertex.z -= 0.001f; }
    auto lines = create_geometry(Lines, edge_vertices, edge_indices);

    glm::vec4 vertex_color{ 1.0f, 0.0f, 0.0f, 1.0f };
    auto [vertices, vertex_indices] =
        generate_box_geometry_data(
            Points, width * 1.01f, height * 1.01f, depth * 1.01f, width_segments, height_segments, depth_segments, vertex_color
        );
    for (auto& vertex : vertices) { vertex.z -= 0.001f; }
    auto points = create_geometry(Points, vertices, vertex_indices);

    prepare_for_rendering();
    enable_face_culling();
    enable_depth_test();
    set_line_width(3);

    static const float CAMERA_SPEED{ 0.1f };
    static const float CAMERA_ROT_SPEED{ 0.01f };
    static const float CAMERA_NEAR_PLANE{ 0.01f };
    static const float CAMERA_FAR_PLANE{ 100.0f };
    static const float CAMERA_FOV{ 1.13f };
    glm::vec3 camera_position{ -0.9f, 0.8f, 1.6f };
    glm::vec3 camera_rotation{ -0.5f, -0.55f, 0.0f };

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

        set_geometry_current(&lines);
        render_current_geometry();

        set_geometry_current(&points);
        render_current_geometry();

        set_geometry_current(&triangles);
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
