#include "asr.h"

#include <cstddef>
#include <string>
#include <vector>

static const std::string Vertex_Shader_Source = R"(
    #version 110

    attribute vec4 position;
    attribute vec4 color;

    uniform float time;

    varying vec4 fragment_color;

    void main()
    {
        fragment_color = color;
        gl_Position = position;
    }
)";

static const std::string Fragment_Shader_Source = R"(
    #version 110

    varying vec4 fragment_color;

    void main()
    {
        gl_FragColor = fragment_color;
    }
)";

std::vector<float> generate_circle_geometry_data(
    float radius,
    unsigned int segment_count
)
{
    std::vector<float> vertices;

    float angle = 0.0f;
    float angle_delta = static_cast<float>(M_PI)*2.0f/segment_count;

    for (unsigned int i = 0; i < segment_count; i++)
    {
        vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
        vertices.insert(vertices.end(), { 1.0f, 1.0f, 1.0f, 1.0f });


        vertices.insert(vertices.end(), { cosf(angle) * radius , sinf(angle) * radius, 0.0f });
        vertices.insert(vertices.end(), { 1.0f, 1.0f, 1.0f, 1.0f });

        angle = angle + angle_delta;

        vertices.insert(vertices.end(), { cosf(angle) * radius, sinf(angle) * radius, 0.0f });
        vertices.insert(vertices.end(), { 1.0f, 1.0f, 1.0f, 1.0f });
    }

    return vertices;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    using namespace asr;

    create_window(500, 500, "Circle Test on ASR Version 1.0");
    create_shader(Vertex_Shader_Source, Fragment_Shader_Source);

    auto geometry = generate_circle_geometry_data(0.5f, 60);
    size_t geometry_vertex_count{ geometry.size()/7 };
    create_geometry(GeometryType::Triangles, &geometry[0], geometry_vertex_count);
    
    prepare_for_rendering();
    bool should_stop = false;

    while (!should_stop) {
        process_window_events(&should_stop);
        render_next_frame();
    }

    destroy_geometry();
    destroy_shader();
    destroy_window();

    return 0;
}
