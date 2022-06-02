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

std::vector<float> generate_rectangle_geometry_data(
    float width,
    float height,
    unsigned int width_segments_count,
    unsigned int height_segments_count
)
{
    std::vector<float> vertices;

    float h = (height * height_segments_count) / 2.0f;
    float w = (width * width_segments_count) / 2.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    for (unsigned int i = 0; i < width_segments_count; i++)
    {
        for (unsigned int j = 0; j < height_segments_count; j++)
        {
            float x { -w + i * width };
            float y { h - j * height };

            vertices.insert(vertices.end(), { x, y, 0.0f });
            vertices.insert(vertices.end(), { r, g, b, a });
            vertices.insert(vertices.end(), { x + width, y, 0.0f });
            vertices.insert(vertices.end(), { r, g, b, a });
            vertices.insert(vertices.end(), { x, y - height, 0.0f });
            vertices.insert(vertices.end(), { r, g, b, a });

            vertices.insert(vertices.end(), { x + width, y - height, 0.0f });
            vertices.insert(vertices.end(), { r, g, b, a });
            vertices.insert(vertices.end(), { x + width,  y, 0.0f });
            vertices.insert(vertices.end(), { r, g, b, a });
            vertices.insert(vertices.end(), { x, y - height, 0.0f });
            vertices.insert(vertices.end(), { r, g, b, a });
        }
    }

    return vertices;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    using namespace asr;

    create_window(500, 500, "Rectangle Test on ASR Version 1.0");
    create_shader(Vertex_Shader_Source, Fragment_Shader_Source);

    auto geometry = generate_rectangle_geometry_data(0.2f, 0.2f, 5, 5);

    size_t geometry_vertex_count{ geometry.size() / 7};
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
