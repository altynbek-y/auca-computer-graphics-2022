#include "asr.h"

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
        float v{ static_cast<float>(i) / static_cast<float>(height_segments_count) };

        float phi{ v * asr::pi };

        for (auto j = 0U; j <= width_segments_count; ++j) {
            float u{ static_cast<float>(j) / static_cast<float>(width_segments_count) };

            float theta{ u * asr::two_pi };

            float cos_phi{ std::cosf(phi) };
            float sin_phi{ std::sinf(phi) };
            float cos_theta{ std::cosf(theta) };
            float sin_theta{ std::sinf(theta) };

            float x{ cos_theta * sin_phi };
            float y{ cos_phi };
            float z{ sin_phi * sin_theta };

            vertices.push_back(asr::Vertex{
                x * radius, y * radius, z * radius,
                color.r, color.g, color.b, color.a,
                1.0f - u, v
            });

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
                    indices.insert(indices.end(), { index_b, index_d, index_d, index_c, index_c, index_b });
                }
                else {
                    if (rows != 0)
                    {
                        indices.insert(indices.end(), { index_a, index_b, index_c });
                    } if (rows != height_segments_count - 1)
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

    create_window(1500, 900, "Transform Test 1 on ASR Version 1.2");
    create_shader(Vertex_Shader_Source, Fragment_Shader_Source);

    float radius{ 0.5f };
    unsigned int width_segments{ 30 }, height_segments{ 30 };

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

    glm::vec3 sphere_position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 sphere_rotation{ 0.0f, 0.01f, 0.0f };
    glm::vec3 sphere_scale{ 1.0f, 1.0f, 1.0f };

    auto image_sun = read_image_file("data/images/sun.jpg");
    auto texture_sun = generate_texture(image_sun);

    auto image_venus = read_image_file("data/images/venus.jpg");
    auto texture_venus = generate_texture(image_venus);

    auto image_earth = read_image_file("data/images/earth.jpg");
    auto texture_earth = generate_texture(image_earth);

    auto image_moon = read_image_file("data/images/moon.jpg");
    auto texture_moon = generate_texture(image_moon);

    prepare_for_rendering();
    enable_face_culling();
    enable_depth_test();
    set_line_width(3);

    static const float CAMERA_SPEED{ 6.0f };
    static const float CAMERA_ROT_SPEED{ 1.5f };
    static const float CAMERA_FOV{ 1.13f };
    static const float CAMERA_NEAR_PLANE{ 0.1f };
    static const float CAMERA_FAR_PLANE{ 100.0f };

    glm::vec3 camera_position{ 1.40f, 1.0f, 1.5f };
    glm::vec3 camera_rotation{ -0.5f, 0.75f, 0.0f };
    set_keys_down_event_handler([&](const uint8_t* keys) {
        if (keys[SDL_SCANCODE_ESCAPE]) std::exit(0);
        if (keys[SDL_SCANCODE_W]) camera_rotation.x -= CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_A]) camera_rotation.y += CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_S]) camera_rotation.x += CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_D]) camera_rotation.y -= CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_UP]) {
            glm::vec3 shift = (get_view_matrix() * glm::vec4{ 0.0f, 0.0f, 1.0f, 0.0f } *(CAMERA_SPEED * get_dt())).xyz();
            camera_position -= shift;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            glm::vec3 shift = (get_view_matrix() * glm::vec4{ 0.0f, 0.0f, 1.0f, 0.0f } *(CAMERA_SPEED * get_dt())).xyz();
            camera_position += shift;
        }
        });
    set_matrix_mode(MatrixMode::Projection);
    load_perspective_projection_matrix(CAMERA_FOV, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);

    float sun_scale{ 2.0f };
    float venus_scale{ 0.4f };
    float venus_sun_distance{ 2.7f };
    float venus_sun_angle{0.0f};
    float venus_sun_angle_delta{ 0.01f };

    float earth_scale{ 0.4f };
    float earth_sun_distance{ 4.3f };
    float earth_sun_angle{ 0.0f };
    float earth_sun_angle_delta{ -0.005f };

    float moon_scale{ 0.4f };
    float moon_earth_distance{ 1.1f };
    float moon_earth_angle{ 0.0f };
    float moon_earth_angle_delta{ 0.07f };

    bool should_stop{ false };

    while (!should_stop) {
        process_window_events(&should_stop);

        prepare_to_render_frame();

        set_matrix_mode(MatrixMode::View);
        load_identity_matrix();
        translate_matrix(camera_position);
        rotate_matrix(camera_rotation);
        
        // Sun
        set_matrix_mode(MatrixMode::Model);

        push_matrix();
        {
            scale_matrix(glm::vec3{ sun_scale });

            set_texture_current(&texture_sun);
            set_geometry_current(&triangles);
            render_current_geometry();

            set_texture_current(nullptr);
            render_current_geometry();
        }
        pop_matrix();

        // Venus
        push_matrix();
        {
            rotate_matrix(glm::vec3{ 0.0f, venus_sun_angle, 0.0f });
            translate_matrix(glm::vec3{ venus_sun_distance, 0.0f, 0.0f });

            scale_matrix(glm::vec3{ venus_scale });

            set_texture_current(&texture_venus);
            set_geometry_current(&triangles);
            render_current_geometry();

            set_texture_current(nullptr);
            render_current_geometry();

            venus_sun_angle += venus_sun_angle_delta;
        }
        pop_matrix();

        // Earth
        push_matrix();
        {
            rotate_matrix(glm::vec3{ 0.0f, earth_sun_angle, 0.0f });
            translate_matrix(glm::vec3{ earth_sun_distance, 0.0f, 0.0f });

            scale_matrix(glm::vec3{ earth_scale });

            set_texture_current(&texture_earth);
            set_geometry_current(&triangles);
            render_current_geometry();

            set_texture_current(nullptr);
            render_current_geometry();

            earth_sun_angle += earth_sun_angle_delta;

            // Moon
            push_matrix();
            {
                rotate_matrix(glm::vec3{ 0.0f, moon_earth_angle, 0.0f });
                translate_matrix(glm::vec3{ moon_earth_distance, 0.0f, 0.0f });
                scale_matrix(glm::vec3{ moon_scale });

                set_texture_current(&texture_moon);
                set_geometry_current(&triangles);
                render_current_geometry();

                set_texture_current(nullptr);
                render_current_geometry();

                moon_earth_angle += moon_earth_angle_delta;
            }
            pop_matrix();
        }
        pop_matrix();

        finish_frame_rendering();
    }

    destroy_geometry(triangles);
    destroy_geometry(lines);
    destroy_geometry(points);

    destroy_shader();
    destroy_window();

    return 0;
}
