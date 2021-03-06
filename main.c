#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define OPENGL_WIDTH 800
#define OPENGL_HEIGHT 600

uint8_t pixels[4 * OPENGL_WIDTH * OPENGL_HEIGHT];
char frame_file_path[1024];

void save_pixels_to_ppm_file(const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (!f) {
        fprintf(stderr, "Could not save to file `%s`: %s",
                file_path, strerror(errno));
        exit(1);
    }

    fprintf(f, "P6\n%d %d\n%d\n", OPENGL_WIDTH, OPENGL_HEIGHT, 255);
    for (int y = 0; y < OPENGL_HEIGHT; ++y) {
        for (int x = 0; x < OPENGL_WIDTH; ++x) {
            fputc(pixels[(OPENGL_HEIGHT - y - 1) * OPENGL_WIDTH * 4 + x * 4 + 0], f);
            fputc(pixels[(OPENGL_HEIGHT - y - 1) * OPENGL_WIDTH * 4 + x * 4 + 1], f);
            fputc(pixels[(OPENGL_HEIGHT - y - 1) * OPENGL_WIDTH * 4 + x * 4 + 2], f);
        }
    }
    fclose(f);
}

const char *gl_shader_type_as_cstr(GLenum shader_type)
{
    switch (shader_type) {
    case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
    case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
    default: return "UNKNOWN";
    }
}

const char *file_as_cstr(const char *filepath)
{
    assert(filepath);

    size_t n = 0;
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "Could not open file `%s`: %s\n", filepath,
                strerror(errno));
        abort();
    }

    int code = fseek(f, 0, SEEK_END);
    if (code < 0) {
        fprintf(stderr, "Could find the end of file %s: %s\n", filepath,
                strerror(errno));
        abort();
    }

    long m = ftell(f);
    if (m < 0) {
        fprintf(stderr, "Could get the end of file %s: %s\n", filepath,
                strerror(errno));
        abort();
    }
    size_t count = (size_t) m;

    code = fseek(f, 0, SEEK_SET);
    if (code < 0) {
        fprintf(stderr, "Could not find the beginning of file %s: %s\n", filepath,
                strerror(errno));
        abort();
    }

    char *buffer = calloc(1, count + 1);
    if (!buffer) {
        fprintf(stderr, "Could not allocate memory for file %s: %s\n", filepath,
                strerror(errno));
        abort();
    }

    n = fread(buffer, 1, count, f);
    if (n != count) {
        fprintf(stderr, "Could not read file %s: %s\n", filepath,
                strerror(errno));
        abort();
    }

    return buffer;
}

GLuint gl_create_and_compile_shader(GLenum shader_type, const char *file_path)
{
    const char *source = file_as_cstr(file_path);

    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        const size_t MESSAGE_CAPACITY = 1024;
        GLsizei log_length = 0;
        GLchar message[MESSAGE_CAPACITY];
        glGetShaderInfoLog(shader, MESSAGE_CAPACITY, &log_length, message);

        fprintf(stderr, "%s failed compiled:\n", gl_shader_type_as_cstr(shader_type));
        fprintf(stderr, "%s:%.*s", file_path, log_length - 2, message + 2);
        abort();
    }

    return shader;
}

GLuint gl_create_and_link_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        const size_t MESSAGE_CAPACITY = 1024;
        GLsizei log_length = 0;
        GLchar message[MESSAGE_CAPACITY];
        glGetProgramInfoLog(program, MESSAGE_CAPACITY, &log_length, message);
        fprintf(stderr, "Program failed linkage: %.*s", log_length, message);
        abort();
    }

    return program;
}

void window_size_callback(GLFWwindow* window,
                          int width, int height)
{
    glViewport(0, 0, width, height);
}

#define VERTEX_FILE_PATH "shader.vert"
#define FRAGMENT_FILE_PATH "shader.frag"

int main(int argc, char *argv[])
{
    if (!glfwInit()) {
        fprintf(stderr, "Error: Could not initialize GLFW!\n");
        abort();
    }

    GLFWwindow *window = glfwCreateWindow(
        OPENGL_WIDTH, OPENGL_HEIGHT, "OpenGL in 2020 KEKW",
        NULL, NULL);

    if (!window) {
        fprintf(stderr, "Error: Could not create GLFW Window!\n");
        glfwTerminate();
        abort();
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, window_size_callback);
    glfwSwapInterval(1);

    float mesh[6][4] = {
        // position,    texcoord
        {-1.0f, -1.0f, 0.0f, 1.0f},
        { 1.0f, -1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 1.0f, 0.0f},
        { 1.0f,  1.0f, 1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f, 1.0f},
    };

    const size_t mesh_count = sizeof(mesh) / sizeof(mesh[0]);
    const size_t mesh_component_count = sizeof(mesh[0]) / sizeof(mesh[0][0]);

    // BUFFERS
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mesh), mesh, GL_STATIC_DRAW);

    const GLuint position_index = 0;
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, mesh_component_count, GL_FLOAT, GL_FALSE, 0, NULL);

    // TEXTURES
    GLuint texture = 0;
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const char *image_filename = "./cakew.png";
    int wi, hi, ni;
    unsigned char *image_pixels = stbi_load(image_filename, &wi, &hi, &ni, 4);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 wi,
                 hi,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 image_pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    // SHADERS
    GLuint vertex_shader = gl_create_and_compile_shader(
        GL_VERTEX_SHADER, VERTEX_FILE_PATH);

    // TODO: why tf gl_FragCoord is vec4? Does it use z as part of the z-buffer?
    GLuint fragment_shader = gl_create_and_compile_shader(
        GL_FRAGMENT_SHADER, FRAGMENT_FILE_PATH);

    GLuint program = gl_create_and_link_program(vertex_shader, fragment_shader);

    // TODO: clean up stuff properly
    //    Deleting probably requires detaching the shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glUseProgram(program);

    float r = 50.0f;
    float x = 300.0f;
    float y = 300.0f;
    float dx = 200.0f;
    float dy = 200.0f;

    GLint position_location = glGetUniformLocation(program, "position");
    GLint direction_location = glGetUniformLocation(program, "direction");
    GLint resolution_location = glGetUniformLocation(program, "resolution");
    GLint dt_location = glGetUniformLocation(program, "dt");
    GLint radius_location = glGetUniformLocation(program, "RADIUS");

    int w, h;
    glfwGetWindowSize(window, &w, &h);

    const float dt = 1.0f / 60.0f;
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    glUniform2f(position_location, x, y);
    glUniform2f(direction_location, dx, dy);
    glUniform2f(resolution_location, (float) w, (float) h);
    glUniform1f(dt_location, dt);
    glUniform1f(radius_location, r);

    for (int i = 0; i < 100 && !glfwWindowShouldClose(window); ++i) {
        glfwGetWindowSize(window, &w, &h);

        glClearColor(0.0, 0.0f, 0.75f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if ((x - r) <= 0.0f || (x + r) >= (float) w) dx = -dx;
        if ((y - r) <= 0.0f || (y + r) >= (float) h) dy = -dy;
        x += dx * dt;
        y += dy * dt;

        glUniform2f(position_location, x, y);
        glUniform2f(direction_location, dx, dy);
        glUniform2f(resolution_location, (float) w, (float) h);
        glUniform1f(dt_location, dt);

        glDrawArrays(GL_TRIANGLES, position_index, mesh_count);


#define OFFSCREEN
#ifdef OFFSCREEN
        glReadPixels(0, 0, OPENGL_WIDTH, OPENGL_HEIGHT,
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     pixels);

        snprintf(frame_file_path, sizeof(frame_file_path), "./frames/frame-%04d.ppm", i);
        save_pixels_to_ppm_file(frame_file_path);
#endif // OFFSCREEN

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
