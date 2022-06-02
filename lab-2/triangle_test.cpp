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
        gl_PointSize = 7.0;
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

static asr::GeometryPair generate_triangle_geometry_data(
                            asr::GeometryType geometryType,
                            float width, 
                            float height,
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

    float half_height{ height * 0.5f };
    float segment_height{ height / static_cast<float>(height_segments_count) }; 

    float half_width{ width * 0.5f };
    float segment_width{ width / static_cast<float>(width_segments_count) }; 

    float segment_half_width { segment_width * 0.5f};

    for (float i = 0.0f, offset = 0.0f; i < height_segments_count; i++, offset -= segment_half_width) {
        for (unsigned int j = 0; j <= i; j++) {

            float x1{ offset + segment_width * j };
            float y1{ half_height - i * segment_height };

            float x2{ offset - segment_half_width + segment_width * j };
            float y2{ half_height - (i + 1) * segment_height };

            float x3{ offset + segment_half_width + segment_width * j };
            float y3{ half_height - (i + 1) * segment_height };

            float x4{ offset + (j - 1) * segment_width };

            vertices.push_back(asr::Vertex{ x1, y1, 0.0f, color.r, color.g, color.b, color.a });
            if (geometryType == asr::GeometryType::Points) {
                indices.push_back(vertices.size() - 1);
            }
            vertices.push_back(asr::Vertex{ x2, y2, 0.0f, color.r, color.g, color.b, color.a });
            if (geometryType == asr::GeometryType::Points) {
                indices.push_back(vertices.size() - 1);
            }
            vertices.push_back(asr::Vertex{ x3, y3, 0.0f, color.r, color.g, color.b, color.a });
            if (geometryType == asr::GeometryType::Points) {
                indices.push_back(vertices.size() - 1);
            }

            if (j % 2 || j % 3 || j % 11) {
                vertices.push_back(asr::Vertex{ x1, y1, 0.0f, color.r, color.g, color.b, color.a });
                vertices.push_back(asr::Vertex{ x4, y1, 0.0f,color.r, color.g, color.b, color.a });
                vertices.push_back(asr::Vertex{ x2, y2, 0.0f, color.r, color.g, color.b, color.a });
            }
        }
    }

    if (geometryType == asr::GeometryType::Lines || geometryType == asr::GeometryType::Triangles) {
        for (auto j = 0U; j <= height_segments_count * width_segments_count*3; j+=3) {

            unsigned int index_a{ j };
            unsigned int index_b{ index_a + 1 };
            unsigned int index_c{ index_a + 2 };

            if (geometryType == asr::GeometryType::Lines) {
                    indices.insert(indices.end(), { index_a, index_b, index_b, index_c, index_c, index_a });
            } else {
                    indices.insert(indices.end(), { index_a, index_b, index_c });
            }
        }
    }

    return std::make_pair(vertices, indices);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    using namespace asr;

    create_window(500, 500, "Triangle Test on ASR Version 1.1");
    create_shader(Vertex_Shader_Source, Fragment_Shader_Source);

    float width{ 1.0f }, height{ 1.0f };
    unsigned int width_segments{ 5 }, height_segments{ 5 };

    auto [triangle_vertices, triangle_indices] =
        generate_triangle_geometry_data(
            Triangles, width, height, width_segments, height_segments
        );
    auto triangles =
        create_geometry(
            Triangles, triangle_vertices, triangle_indices
        );

    glm::vec4 edge_color{ 1.0f, 0.7f, 0.7f, 1.0f };
    auto [edge_vertices, edge_indices] =
        generate_triangle_geometry_data(
            Lines, width, height, width_segments, height_segments, edge_color
        );
    for (auto& vertex : edge_vertices) { vertex.z -= 0.01f; }
    auto lines = create_geometry(Lines, edge_vertices, edge_indices);

    glm::vec4 vertex_color{ 1.0f, 0.0f, 0.0f, 1.0f };
    auto [vertices, vertex_indices] =
        generate_triangle_geometry_data(
            Points, width, height, width_segments, height_segments, vertex_color
        );
    for (auto& vertex : vertices) { vertex.z -= 0.02f; }
    auto points = create_geometry(Points, vertices, vertex_indices);

    prepare_for_rendering();
    enable_face_culling();
    enable_depth_test();
    set_line_width(2);

    bool should_stop{ false };
    while (!should_stop) {
        process_window_events(&should_stop);

        prepare_to_render_frame();

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