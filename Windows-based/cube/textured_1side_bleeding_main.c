#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define DEG2RAD(x) ((x) * 0.017453292519943295769f)

// --- Matrix utils ---
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
    m[12] = x; m[13] = y; m[14] = z;
}
static void mat_rotate(float *m, float angle, float x, float y, float z) {
    float c = cosf(angle), s = sinf(angle);
    float len = sqrtf(x*x + y*y + z*z);
    if (len == 0.0f) { mat_identity(m); return; }
    x/=len; y/=len; z/=len;
    mat_identity(m);
    m[0]=x*x*(1-c)+c;   m[4]=x*y*(1-c)-z*s; m[8] =x*z*(1-c)+y*s;
    m[1]=y*x*(1-c)+z*s; m[5]=y*y*(1-c)+c;   m[9] =y*z*(1-c)-x*s;
    m[2]=x*z*(1-c)-y*s; m[6]=y*z*(1-c)+x*s; m[10]=z*z*(1-c)+c;
}

// --- BMP Loader ---
GLuint LoadBMP(const char *imagepath) {
    unsigned char header[54];
    unsigned int dataPos, imageSize, width, height;
    unsigned char *data;
    FILE *file = fopen(imagepath, "rb");
    if (!file) { printf("Image could not be opened\n"); return 0; }
    if (fread(header, 1, 54, file) != 54) { printf("Not a correct BMP file\n"); return 0; }
    if (header[0] != 'B' || header[1] != 'M') { printf("Not a correct BMP file\n"); return 0; }
    if (*(int*)&(header[0x1E]) != 0 || *(int*)&(header[0x1C]) != 24) { printf("Not a 24bpp BMP\n"); return 0; }
    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    width = *(int*)&(header[0x12]);
    height = *(int*)&(header[0x16]);
    if (imageSize == 0) imageSize = width * height * 3;
    if (dataPos == 0) dataPos = 54;
    data = malloc(imageSize);
    fread(data, 1, imageSize, file);
    fclose(file);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    free(data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    return textureID;
}

// --- Shader helper ---
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
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *win = glfwCreateWindow(800, 600, "Cube - One Textured Face", NULL, NULL);
    if (!win) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glewExperimental = GL_TRUE;
    glewInit();

    const char *vshader =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;"
        "layout(location=1) in vec3 aColor;"
        "layout(location=2) in vec2 aTexCoord;"
        "layout(location=3) in float aIsTextured;"
        "out vec3 vColor;"
        "out vec2 vTexCoord;"
        "out float vIsTextured;"
        "uniform mat4 MVP;"
        "void main(){"
        " gl_Position = MVP * vec4(aPos,1.0);"
        " vColor = aColor;"
        " vTexCoord = aTexCoord;"
        " vIsTextured = aIsTextured;"
        "}";

    const char *fshader =
        "#version 330 core\n"
        "in vec3 vColor;"
        "in vec2 vTexCoord;"
        "in float vIsTextured;"
        "out vec4 FragColor;"
        "uniform sampler2D texSampler;"
        "void main(){"
        " vec4 baseColor = vec4(vColor, 1.0);"
        " if (vIsTextured > 0.5)"
        "     FragColor = texture(texSampler, vTexCoord);"
        " else"
        "     FragColor = baseColor;"
        "}";

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vshader);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fshader);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // --- Vertex data ---
    float vertices[] = {
        // position        // color        // texcoords // isTextured
        // Front face (textured)
        -1,-1, 1, 1,1,1, 0,0, 1,
         1,-1, 1, 1,1,1, 1,0, 1,
         1, 1, 1, 1,1,1, 1,1, 1,
        -1, 1, 1, 1,1,1, 0,1, 1,
        // Back face (colored only)
        -1,-1,-1, 1,0,0, 0,0, 0,
         1,-1,-1, 0,1,0, 0,0, 0,
         1, 1,-1, 0,0,1, 0,0, 0,
        -1, 1,-1, 1,1,0, 0,0, 0
    };

    unsigned int indices[] = {
        0,1,2, 2,3,0,   // front (textured)
        4,5,6, 6,7,4,   // back (colored)
        3,2,6, 6,7,3,   // top
        0,1,5, 5,4,0,   // bottom
        1,2,6, 6,5,1,   // right
        0,3,7, 7,4,0    // left
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

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,9*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,9*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,9*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,9*sizeof(float),(void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3);

    glEnable(GL_DEPTH_TEST);
    GLuint texID = LoadBMP("dinosaur.bmp");
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "texSampler"), 0);

    GLint mvpLoc = glGetUniformLocation(prog, "MVP");

    while (!glfwWindowShouldClose(win)) {
        glClearColor(0.2f,0.2f,0.2f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        float t = (float)glfwGetTime();
        float rot[16], view[16], proj[16], mv[16], mvp[16];
        mat_rotate(rot, t, 1.0f, 1.0f, 0.5f);
        mat_translate(view, 0.0f, 0.0f, -6.0f);
        mat_perspective(proj, 45.0f, 800.0f/600.0f, 0.1f, 100.0f);
        mat_multiply(mv, rot, view);
        mat_multiply(mvp, mv, proj);

        glUseProgram(prog);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
