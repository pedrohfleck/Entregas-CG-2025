#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// Janela
const GLuint WINDOW_WIDTH = 1000, WINDOW_HEIGHT = 1000;

// Estados globais
bool rotatingX = false, rotatingY = false, rotatingZ = false;
glm::vec3 objectTranslation(0.0f);
float objectScale = 1.0f;

GLuint modelVAO;
int modelVertices = 0;

// === Função para carregar arquivos OBJ ===
int loadSimpleOBJ(string filePATH, int &nVertices) {
    vector<glm::vec3> vertices;
    vector<GLfloat> vBuffer;
    glm::vec3 color(1.0f, 0.0f, 0.0f);  // vermelho

    ifstream file(filePATH);
    if (!file.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << filePATH << endl;
        return -1;
    }

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        } else if (word == "f") {
            while (ss >> word) {
                int vi = 0;
                stringstream faceSS(word);
                string index;
                if (getline(faceSS, index, '/')) {
                    vi = !index.empty() ? stoi(index) - 1 : 0;
                }
                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }

    file.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 6;
    return VAO;
}

// === Shaders ===
const char* vertexShaderSource = R"(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    vertexColor = vec4(color, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 450 core
in vec4 vertexColor;
out vec4 fragColor;
void main() {
    fragColor = vertexColor;
}
)";

void handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
GLuint createShaderProgram();
void configureOpenGL(GLFWwindow* window);

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vivencial 1 - Evelyn, Pedro e Thiago", nullptr, nullptr);
    configureOpenGL(window);

    cout << "=== Controles ===\n"
         << "X / Y / Z - Alternar rotação\n"
         << "W/A/S/D/I/J - Translação\n"
         << "[ e ] - Escala -/+\n"
         << "ESC - Sair\n";

    GLuint shader = createShaderProgram();

    // Carregar modelo
    modelVAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", modelVertices);
    if (modelVAO == -1) return -1;

    // Uniforms
    GLint modelLoc = glGetUniformLocation(shader, "model");
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -8));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

    // Offsets dos objetos
    vector<glm::vec3> objOffsets = {
        {-2.0f, 0.0f, 0.0f},
        { 0.0f, 0.0f, 0.0f},
        { 2.0f, 0.0f, 0.0f}
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        for (const auto& offset : objOffsets) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, offset + objectTranslation);
            model = glm::scale(model, glm::vec3(objectScale));

            float time = glfwGetTime();
            if (rotatingX) model = glm::rotate(model, time, glm::vec3(1, 0, 0));
            if (rotatingY) model = glm::rotate(model, time, glm::vec3(0, 1, 0));
            if (rotatingZ) model = glm::rotate(model, time, glm::vec3(0, 0, 1));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(modelVAO);
            glDrawArrays(GL_TRIANGLES, 0, modelVertices);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &modelVAO);
    glfwTerminate();
    return 0;
}

void configureOpenGL(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, handleKeyboard);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
}

void handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, true); break;
        case GLFW_KEY_X: rotatingX = !rotatingX; break;
        case GLFW_KEY_Y: rotatingY = !rotatingY; break;
        case GLFW_KEY_Z: rotatingZ = !rotatingZ; break;
        case GLFW_KEY_W: objectTranslation.z -= 0.1f; break;
        case GLFW_KEY_S: objectTranslation.z += 0.1f; break;
        case GLFW_KEY_A: objectTranslation.x -= 0.1f; break;
        case GLFW_KEY_D: objectTranslation.x += 0.1f; break;
        case GLFW_KEY_I: objectTranslation.y += 0.1f; break;
        case GLFW_KEY_J: objectTranslation.y -= 0.1f; break;
        case GLFW_KEY_LEFT_BRACKET: objectScale = max(0.1f, objectScale - 0.1f); break;
        case GLFW_KEY_RIGHT_BRACKET: objectScale += 0.1f; break;
    }
}

GLuint createShaderProgram() {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}
