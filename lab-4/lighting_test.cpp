#include "asr.h"

#include <string>
#include <utility>
#include <vector>
#include <SDL.h>
#undef main


static const std::string Vertex_Shader_Source{R"( // NOLINT(cert-err58-cpp)
    #version 110

    attribute vec4 position;
    attribute vec3 normal;
    attribute vec4 color;
    attribute vec4 texture_coordinates;

    uniform mat4 model_view_matrix;
    uniform mat4 projection_matrix;
    uniform mat3 normal_matrix;

    uniform float point_size;

    uniform bool texture_enabled;
    uniform mat4 texture_transformation_matrix;

    varying vec4 fragment_view_position;
    varying vec3 fragment_view_direction;
    varying vec4 fragment_color;
    varying vec2 fragment_texture_coordinates;
    varying vec3 fragment_view_normal;

    void main()
    {
        vec4 view_position = model_view_matrix * position;
        fragment_view_position = view_position;
        fragment_view_direction = -view_position.xyz;
        fragment_view_normal = normalize(normal_matrix * normal);

        fragment_color = color;
        if (texture_enabled) {
            vec4 transformed_texture_coordinates = texture_transformation_matrix * vec4(texture_coordinates.st, 0.0, 1.0);
            fragment_texture_coordinates = vec2(transformed_texture_coordinates);
        }

        gl_Position = projection_matrix * view_position;
        gl_PointSize = point_size;
    }
)"};

static const std::string Fragment_Shader_Source{R"( // NOLINT(cert-err58-cpp)
    #define TEXTURING_MODE_ADDITION            0
    #define TEXTURING_MODE_SUBTRACTION         1
    #define TEXTURING_MODE_REVERSE_SUBTRACTION 2
    #define TEXTURING_MODE_MODULATION          3
    #define TEXTURING_MODE_DECALING            4

    uniform vec3 material_ambient_color;
    uniform vec4 material_diffuse_color;
    uniform vec4 material_emission_color;
    uniform vec3 material_specular_color;
    uniform float material_specular_exponent;

    uniform bool point_light_enabled;
    uniform bool point_light_two_sided;
    uniform vec3 point_light_view_position;
    uniform vec3 point_light_ambient_color;
    uniform vec3 point_light_diffuse_color;
    uniform vec3 point_light_specular_color;
    uniform float point_light_intensity;
    uniform float point_light_constant_attenuation;
    uniform float point_light_linear_attenuation;
    uniform float point_light_quadratic_attenuation;

    uniform bool point_light2_enabled;
    uniform bool point_light2_two_sided;
    uniform vec3 point_light2_view_position;
    uniform vec3 point_light2_ambient_color;
    uniform vec3 point_light2_diffuse_color;
    uniform vec3 point_light2_specular_color;
    uniform float point_light2_intensity;
    uniform float point_light2_constant_attenuation;
    uniform float point_light2_linear_attenuation;
    uniform float point_light2_quadratic_attenuation;

    uniform bool texture_enabled;
    uniform int texturing_mode;
    uniform sampler2D texture_sampler;

    varying vec4 fragment_view_position;
    varying vec3 fragment_view_direction;
    varying vec3 fragment_view_normal;
    varying vec4 fragment_color;
    varying vec2 fragment_texture_coordinates;

    void main()
    {
        vec3 view_direction = normalize(fragment_view_direction);
        vec3 view_normal = normalize(fragment_view_normal);

        vec4 front_color = material_emission_color;
        front_color.rgb += material_ambient_color;
        front_color.a += material_diffuse_color.a;

        vec4 back_color = front_color;

        if (point_light_enabled) {
            vec3 point_light_vector = point_light_view_position + fragment_view_direction;

            float point_light_vector_length = length(point_light_vector);
            point_light_vector /= point_light_vector_length;

            float point_light_vector_length_squared = point_light_vector_length * point_light_vector_length;
            float attenuation_factor =
                (1.0 / (point_light_constant_attenuation                              +
                        point_light_linear_attenuation    * point_light_vector_length +
                        point_light_quadratic_attenuation * point_light_vector_length_squared));
            attenuation_factor *= point_light_intensity;

            float n_dot_l = max(dot(view_normal, point_light_vector), 0.0);
            vec3 diffuse_color = material_diffuse_color.rgb * point_light_diffuse_color;
            vec3 diffuse_term = n_dot_l * diffuse_color;

            vec3 reflection_vector = reflect(-point_light_vector, view_normal);
            float n_dot_h = clamp(dot(view_direction, reflection_vector), 0.0, 1.0);
            vec3 specular_color = material_specular_color.rgb * point_light_specular_color;
            vec3 specular_term = pow(n_dot_h, material_specular_exponent) * specular_color;

            front_color.rgb += attenuation_factor * (point_light_ambient_color + diffuse_term + specular_term);

            if (point_light_two_sided) {
                vec3 inverted_view_normal = -view_normal;

                n_dot_l = max(dot(-inverted_view_normal, point_light_vector), 0.0);
                diffuse_term = n_dot_l * diffuse_color;

                reflection_vector = reflect(-point_light_vector, inverted_view_normal);
                n_dot_h = clamp(dot(view_direction, reflection_vector), 0.0, 1.0);
                specular_term = pow(n_dot_h, material_specular_exponent) * specular_color;

                back_color.rgb += attenuation_factor * (point_light_ambient_color + diffuse_term + specular_term);
            }
        }

        if (point_light2_enabled) {
            vec3 point_light_vector = point_light2_view_position + fragment_view_direction;

            float point_light_vector_length = length(point_light_vector);
            point_light_vector /= point_light_vector_length;

            float point_light_vector_length_squared = point_light_vector_length * point_light_vector_length;
            float attenuation_factor =
                (1.0 / (point_light2_constant_attenuation                              +
                        point_light2_linear_attenuation    * point_light_vector_length +
                        point_light2_quadratic_attenuation * point_light_vector_length_squared));
            attenuation_factor *= point_light2_intensity;

            float n_dot_l = max(dot(view_normal, point_light_vector), 0.0);
            vec3 diffuse_color = material_diffuse_color.rgb * point_light2_diffuse_color;
            vec3 diffuse_term = n_dot_l * diffuse_color;

            vec3 reflection_vector = reflect(-point_light_vector, view_normal);
            float n_dot_h = clamp(dot(view_direction, reflection_vector), 0.0, 1.0);
            vec3 specular_color = material_specular_color.rgb * point_light2_specular_color;
            vec3 specular_term = pow(n_dot_h, material_specular_exponent) * specular_color;

            front_color.rgb += attenuation_factor * (point_light2_ambient_color + diffuse_term + specular_term);

            if (point_light2_two_sided) {
                vec3 inverted_view_normal = -view_normal;

                n_dot_l = max(dot(-inverted_view_normal, point_light_vector), 0.0);
                diffuse_term = n_dot_l * diffuse_color;

                reflection_vector = reflect(-point_light_vector, inverted_view_normal);
                n_dot_h = clamp(dot(view_direction, reflection_vector), 0.0, 1.0);
                specular_term = pow(n_dot_h, material_specular_exponent) * specular_color;

                back_color.rgb += attenuation_factor * (point_light2_ambient_color + diffuse_term + specular_term);
            }
        }

        gl_FragColor = fragment_color;
        if (gl_FrontFacing) {
            gl_FragColor *= front_color;
        } else {
            gl_FragColor *= back_color;
        }

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
)"};

static asr::GeometryPair generate_rectangle_geometry_data(
                             asr::GeometryType geometry_type,
                             float width, float height,
                             unsigned int width_segments_count,
                             unsigned int height_segments_count,
                             glm::vec4 color = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}
                         )
{
    assert(
        geometry_type == asr::GeometryType::Triangles ||
        geometry_type == asr::GeometryType::Lines     ||
        geometry_type == asr::GeometryType::Points
    );

    asr::Vertices vertices;
    asr::Indices indices;

    float half_height{height * 0.5f};
    float segment_height{height / static_cast<float>(height_segments_count)};

    float half_width{width * 0.5f};
    float segment_width{width / static_cast<float>(width_segments_count)};

    for (auto i = 0U; i <= height_segments_count; ++i) {
        float y{static_cast<float>(i) * segment_height - half_height};
        float v{1.0f - static_cast<float>(i) / static_cast<float>(height_segments_count)};
        for (auto j = 0U; j <= width_segments_count; ++j) {
            float x{static_cast<float>(j) * segment_width - half_width};
            float u{static_cast<float>(j) / static_cast<float>(width_segments_count)};
            vertices.push_back(asr::Vertex{
                x, y, 0.0f,
                0.0f, 0.0f, 1.0f,
                color.r, color.g, color.b, color.a,
                u, v
            });
            if (geometry_type == asr::GeometryType::Points) {
                indices.push_back(vertices.size() - 1);
            }
        }
    }

    if (geometry_type == asr::GeometryType::Lines || geometry_type == asr::GeometryType::Triangles) {
        for (auto i = 0U; i < height_segments_count; ++i) {
            for (auto j = 0U; j < width_segments_count; ++j) {
                unsigned int index_a{i * (width_segments_count + 1) + j};
                unsigned int index_b{index_a + 1};
                unsigned int index_c{index_a + (width_segments_count + 1)};
                unsigned int index_d{index_c + 1};
                if (geometry_type == asr::GeometryType::Lines) {
                    indices.insert(indices.end(), {
                        index_a, index_b, index_b, index_c, index_c, index_a
                    });
                    indices.insert(indices.end(), {
                        index_b, index_d, index_d, index_c, index_c, index_b
                    });
                } else {
                    indices.insert(indices.end(), {index_a, index_b, index_c});
                    indices.insert(indices.end(), {index_b, index_d, index_c});
                }
            }
        }
    }

    return std::make_pair(vertices, indices);
}

static asr::GeometryPair generate_sphere_geometry_data(
                             asr::GeometryType geometry_type,
                             float radius,
                             unsigned int width_segments_count,
                             unsigned int height_segments_count,
                             glm::vec4 color = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f} 
                         )
{
    assert(
        geometry_type == asr::GeometryType::Triangles ||
        geometry_type == asr::GeometryType::Lines     ||
        geometry_type == asr::GeometryType::Points
    );

    asr::Vertices vertices;
    asr::Indices indices;

    for (auto i = 0U; i <= height_segments_count; ++i) {
        float v{static_cast<float>(i) / static_cast<float>(height_segments_count)};
        float phi{v * asr::pi};

        for (auto j = 0U; j <= width_segments_count; ++j) {
            float u{static_cast<float>(j) / static_cast<float>(width_segments_count)};
            float theta{u * asr::two_pi};

            float cos_phi{std::cos(phi)};
            float sin_phi{std::sin(phi)};
            float cos_theta{std::cos(theta)};
            float sin_theta{std::sin(theta)};

            float x{cos_theta * sin_phi};
            float y{cos_phi};
            float z{sin_phi * sin_theta};

            vertices.push_back(asr::Vertex{
                x * radius, y * radius, z * radius,
                x, y, z,
                color.r, color.g, color.b, color.a,
                1.0f - u, v
            });
            if (geometry_type == asr::GeometryType::Points) {
                indices.push_back(vertices.size() - 1);
            }
        }
    }

    if (geometry_type == asr::GeometryType::Lines || geometry_type == asr::GeometryType::Triangles) {
        for (auto rows = 0U; rows < height_segments_count; ++rows) {
            for (auto columns = 0U; columns < width_segments_count; ++columns) {
                unsigned int index_a{rows * (width_segments_count + 1) + columns};
                unsigned int index_b{index_a + 1};
                unsigned int index_c{index_a + (width_segments_count + 1)};
                unsigned int index_d{index_c + 1};
                if (geometry_type == asr::GeometryType::Lines) {
                    if (rows != 0) {
                        indices.insert(indices.end(), {
                            index_a, index_b, index_b, index_c, index_c, index_a
                        });
                    }
                    if (rows != height_segments_count - 1) {
                        indices.insert(indices.end(), {
                            index_b, index_d, index_d, index_c, index_c, index_b
                        });
                    }
                } else {
                    if (rows != 0) {
                        indices.insert(indices.end(), {index_a, index_b, index_c});
                    }
                    if (rows != height_segments_count - 1) {
                        indices.insert(indices.end(), {index_b, index_d, index_c});
                    }
                }
            }
        }
    }

    return std::make_pair(vertices, indices);
}

int main()
{
    using namespace asr;

    create_window("Lighting Test on ASR Version 1.3");

    // Material

    glm::vec3 material_ambient_color{0.0f};
    glm::vec4 material_diffuse_color{1.0f};
    glm::vec4 material_emission_color{0.0f};
    glm::vec3 material_specular_color{1.0f};
    float material_specular_exponent{30.0f};

    bool point_light1_enabled{true};
    float point_light1_height{1.0f};
    glm::vec3 point_light1_ambient_color{0.1f};
    glm::vec3 point_light1_diffuse_color{1.0f, 1.0f, 1.0f};
    glm::vec3 point_light1_specular_color{1.0f};
    float point_light1_intensity{1.0f};
    bool point_light1_two_sided{false};
    float point_light1_constant_attenuation{3.0f};
    float point_light1_linear_attenuation{0.0f};
    float point_light1_quadratic_attenuation{0.0f};
    float point_light1_orbit_angle{0.0f};
    float point_light1_orbit_delta_angle{0.01f};
    float point_light1_orbit_radius{1.0f};

    bool point_light2_enabled{ true };
    float point_light2_height{ 1.0f };
    glm::vec3 point_light2_ambient_color{ 0.1f };
    glm::vec3 point_light2_diffuse_color{ 1.0f, 1.0f, 1.0f };
    glm::vec3 point_light2_specular_color{ 1.0f };
    float point_light2_intensity{ 1.0f };
    bool point_light2_two_sided{ false };
    float point_light2_constant_attenuation{ 3.0f };
    float point_light2_linear_attenuation{ 0.0f };
    float point_light2_quadratic_attenuation{ 0.0f };
    float point_light2_orbit_angle{ 0.0f };
    float point_light2_orbit_delta_angle{ -0.01f };
    float point_light2_orbit_radius{ 1.1f };

    float point_light_intensity_max{1.5f};
    float point_light_intensity_min{0.0f};
    float point_light_intensity_angle{0.0f};
    float point_light_intensity_angle_delta{0.01f};

    auto material = create_material(Vertex_Shader_Source, Fragment_Shader_Source);

    // Plane Geometry

    auto [plane_geometry_vertices, plane_geometry_indices] = generate_rectangle_geometry_data(Triangles, 500.0f, 500.0f, 1U, 1U);
    auto plane_geometry = create_geometry(Triangles, plane_geometry_vertices, plane_geometry_indices);

    // Sphere Geometry

    auto [sphere_geometry_vertices, sphere_geometry_indices] = generate_sphere_geometry_data(Triangles, 0.025f, 40U, 40U);
    auto sphere_geometry = create_geometry(Triangles, sphere_geometry_vertices, sphere_geometry_indices);

    prepare_for_rendering();

    set_material_current(&material);
    set_material_depth_test_enabled(true);
    set_material_face_culling_enabled(false);

    set_material_parameter("material_ambient_color", material_ambient_color);
    set_material_parameter("material_diffuse_color", material_diffuse_color);
    set_material_parameter("material_emission_color", material_emission_color);
    set_material_parameter("material_specular_color", material_specular_color);
    set_material_parameter("material_specular_exponent", material_specular_exponent);

    // Light 1 parameters
    set_material_parameter("point_light_enabled", point_light1_enabled);
    set_material_parameter("point_light_two_sided", point_light1_two_sided);
    set_material_parameter("point_light_ambient_color", point_light1_ambient_color);
    set_material_parameter("point_light_diffuse_color", point_light1_diffuse_color);
    set_material_parameter("point_light_specular_color", point_light1_specular_color);
    set_material_parameter("point_light_intensity", point_light1_intensity);
    set_material_parameter("point_light_constant_attenuation", point_light1_constant_attenuation);
    set_material_parameter("point_light_linear_attenuation", point_light1_linear_attenuation);
    set_material_parameter("point_light_quadratic_attenuation", point_light1_quadratic_attenuation);

    // Light 2 parameters
    set_material_parameter("point_light2_enabled", point_light2_enabled);
    set_material_parameter("point_light2_two_sided", point_light2_two_sided);
    set_material_parameter("point_light2_ambient_color", point_light2_ambient_color);
    set_material_parameter("point_light2_diffuse_color", point_light2_diffuse_color);
    set_material_parameter("point_light2_specular_color", point_light2_specular_color);
    set_material_parameter("point_light2_intensity", point_light2_intensity);
    set_material_parameter("point_light2_constant_attenuation", point_light2_constant_attenuation);
    set_material_parameter("point_light2_linear_attenuation", point_light2_linear_attenuation);
    set_material_parameter("point_light2_quadratic_attenuation", point_light2_quadratic_attenuation);

    // Camera Parameters

    static const float CAMERA_SPEED{6.0f};
    static const float CAMERA_ROT_SPEED{1.5f};
    static const float CAMERA_FOV{1.13f};
    static const float CAMERA_NEAR_PLANE{0.1f};
    static const float CAMERA_FAR_PLANE{100.0f};

    glm::vec3 camera_position{0.0f, 3.4f, 2.1f};
    glm::vec3 camera_rotation{-1.05f, 0.0f, 0.0f};
    set_keys_down_event_handler([&](const uint8_t *keys) {
        if (keys[SDL_SCANCODE_ESCAPE]) std::exit(0);
        if (keys[SDL_SCANCODE_W]) camera_rotation.x -= CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_A]) camera_rotation.y += CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_S]) camera_rotation.x += CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_D]) camera_rotation.y -= CAMERA_ROT_SPEED * get_dt();
        if (keys[SDL_SCANCODE_UP]) {
            glm::vec3 shift = (get_view_matrix() * glm::vec4{0.0f, 0.0f, 1.0f, 0.0f} * (CAMERA_SPEED * get_dt())).xyz();
            camera_position -= shift;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            glm::vec3 shift = (get_view_matrix() * glm::vec4{0.0f, 0.0f, 1.0f, 0.0f} * (CAMERA_SPEED * get_dt())).xyz();
            camera_position += shift;
        }
    });
    set_matrix_mode(Projection);
    load_perspective_projection_matrix(CAMERA_FOV, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);

    // Plane Parameters

    glm::vec3 plane_position{0.0f, 0.0f, 0.0f};
    glm::vec3 plane_rotation{-half_pi, 0.0f, 0.0f};

    // Sphere Parameters

    glm::vec3 sphere_position{0.0f, 0.5f, 0.0f};
    glm::vec3 sphere_scale{20.0f, 20.0f, 20.0f};

    bool should_stop{false};
    while (!should_stop) {
        process_window_events(&should_stop);

        prepare_to_render_frame();

        // Camera

        set_matrix_mode(View);
        load_identity_matrix();
        translate_matrix(camera_position);
        rotate_matrix(camera_rotation);

        // Material

        // Light 1
        glm::vec3 point_light1_position{
            std::cos(point_light1_orbit_angle) * point_light1_orbit_radius,
            point_light1_height,
            std::sin(point_light1_orbit_angle) * point_light1_orbit_radius
        };
        set_material_parameter("point_light_view_position", (get_view_matrix_inverted() * glm::vec4{ point_light1_position, 1.0f}).xyz());
        point_light1_orbit_angle += point_light1_orbit_delta_angle;

        point_light1_intensity = point_light_intensity_min + ((std::sinf(point_light_intensity_angle) + 1.0f) * 0.5f) * (point_light_intensity_max - point_light_intensity_min);        

        // Light 2
        glm::vec3 point_light2_position{
           std::cos(point_light2_orbit_angle) * point_light2_orbit_radius,
           point_light2_height,
           std::sin(point_light2_orbit_angle) * point_light2_orbit_radius
        };
        set_material_parameter("point_light2_view_position", (get_view_matrix_inverted() * glm::vec4{ point_light2_position, 1.0f }).xyz());
        point_light2_orbit_angle += point_light2_orbit_delta_angle;

        point_light2_intensity = point_light_intensity_min + ((std::sinf(point_light_intensity_angle) + 1.0f) * 0.5f) * (point_light_intensity_max - point_light_intensity_min);

        point_light_intensity_angle += point_light_intensity_angle_delta;


        // Plane

        set_material_parameter("material_emission_color", glm::vec4{0.0f, 0.0f, 0.0f, 0.0f});
        set_material_parameter("point_light_enabled", true);

        set_matrix_mode(Model);
        load_identity_matrix();
        translate_matrix(plane_position);
        rotate_matrix(plane_rotation);

        set_geometry_current(&plane_geometry);
        render_current_geometry();

        // Sphere

        load_identity_matrix();
        translate_matrix(sphere_position);
        scale_matrix(sphere_scale);

        set_geometry_current(&sphere_geometry);
        render_current_geometry();

        // Light 1

        set_material_parameter("point_light_enabled", true);
        set_material_parameter("material_emission_color", glm::vec4{ point_light1_diffuse_color, 1.0f});

        load_identity_matrix();
        translate_matrix(point_light1_position);

        set_geometry_current(&sphere_geometry);
        render_current_geometry();

        // Light 2

        set_material_parameter("point_light2_enabled", true);
        set_material_parameter("material_emission_color", glm::vec4{ point_light2_diffuse_color, 1.0f });

        load_identity_matrix();
        translate_matrix(point_light2_position);

        set_geometry_current(&sphere_geometry);
        render_current_geometry();

        finish_frame_rendering();
    }

    destroy_geometry(sphere_geometry);
    destroy_geometry(plane_geometry);

    destroy_material(material);

    destroy_window();

    return 0;
}
