#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0/600.0, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(-0.5f, -0.5f, 0.0f);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex3f( 0.5f, -0.5f, 0.0f);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3f( 0.0f,  0.5f, 0.0f);
        glEnd();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
