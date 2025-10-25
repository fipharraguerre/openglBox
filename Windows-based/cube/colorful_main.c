#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define DEG2RAD(x) ((x) * 0.017453292519943295769f)

// Simple 4x4 matrix utilities (column-major)
static void mat_identity(float *m) {
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void mat_multiply(float *r, const float *a, const float *b) {
    float tmp[16];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            tmp[i*4+j] =
                a[i*4+0]*b[0*4+j] +
                a[i*4+1]*b[1*4+j] +
                a[i*4+2]*b[2*4+j] +
                a[i*4+3]*b[3*4+j];
    for (int i = 0; i < 16; i++) r[i] = tmp[i];
}

static void mat_perspective(float *m, float fov, float aspect, float znear, float zfar) {
    float f = 1.0f / tanf(DEG2RAD(fov) / 2.0f);
    mat_identity(m);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (2.0f * zfar * znear) / (znear - zfar);
    m[15] = 0.0f;
}

static void mat_translate(float *m, float x, float y, float z) {
    mat_identity(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

static void mat_rotate(float *m, float angle, float x, float y, float z) {
    float c = cosf(angle), s = sinf(angle);
    float len = sqrtf(x*x + y*y + z*z);
    if (len == 0.0f) { mat_identity(m); return; }
    x /= len; y /= len; z /= len;
    mat_identity(m);
    m[0] = x*x*(1-c)+c;   m[4] = x*y*(1-c)-z*s; m[8]  = x*z*(1-c)+y*s;
    m[1] = y*x*(1-c)+z*s; m[5] = y*y*(1-c)+c;   m[9]  = y*z*(1-c)-x*s;
    m[2] = x*z*(1-c)-y*s; m[6] = y*z*(1-c)+x*s; m[10] = z*z*(1-c)+c;
}

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "Shader error: %s\n", log);
    }
    return s;
}

int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *win = glfwCreateWindow(800, 600, "Rotating Cube (C)", NULL, NULL);
    if (!win) { fprintf(stderr, "Window creation failed\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "GLEW init failed\n");
        glfwTerminate();
        return -1;
    }

    const char *vshader =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "layout(location=1) in vec3 aColor;\n"
        "out vec3 vColor;\n"
        "uniform mat4 MVP;\n"
        "void main(){\n"
        "  gl_Position = MVP * vec4(aPos,1.0);\n"
        "  vColor = aColor;\n"
        "}\n";

    const char *fshader =
        "#version 330 core\n"
        "in vec3 vColor;\n"
        "out vec4 FragColor;\n"
        "void main(){ FragColor = vec4(vColor,1.0); }\n";

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vshader);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fshader);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    float vertices[] = {
        // positions          // colors
        -1,-1,-1,  1,0,0,  1,-1,-1,  0,1,0,  1, 1,-1,  0,0,1,  -1, 1,-1,  1,1,0,
        -1,-1, 1,  1,0,1,   1,-1, 1,  0,1,1,   1, 1, 1,  1,1,1,  -1, 1, 1,  0,0,0
    };
    unsigned int indices[] = {
        0,1,2, 2,3,0,  1,5,6, 6,2,1,
        5,4,7, 7,6,5,  4,0,3, 3,7,4,
        3,2,6, 6,7,3,  4,5,1, 1,0,4
    };

    GLuint VAO,VBO,EBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);

    GLint mvpLoc = glGetUniformLocation(prog, "MVP");

    while (!glfwWindowShouldClose(win)) {
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        float t = (float)glfwGetTime();
        float model[16], rot[16], view[16], proj[16], mv[16], mvp[16];
        mat_rotate(rot, t, 1.0f, 1.0f, 0.5f);
        mat_translate(view, 0.0f, 0.0f, -6.0f);
        mat_perspective(proj, 45.0f, 800.0f/600.0f, 0.1f, 100.0f);

		mat_multiply(mv, rot, view);    // model * view (reversed)
		mat_multiply(mvp, mv, proj);    // (model * view) * projection (reversed)

        glUseProgram(prog);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
