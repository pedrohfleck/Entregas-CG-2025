// Vivencial1.cpp - Versão extendida para múltiplos objetos com seleção e transformação e cores diferentes

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

// === Classe para representar um objeto 3D ===
struct Object3D {
    GLuint vao;
    int vertexCount;
    glm::vec3 position;
    glm::vec3 rotation;
    float scale;
    glm::vec3 color;
};

vector<Object3D> objects;
int selectedObjectIndex = 0;

// === Função para carregar arquivos OBJ simples ===
// Agora recebe a cor do objeto para aplicar no buffer
int loadSimpleOBJ(string filePATH, int &nVertices, const glm::vec3& color) {
    vector<glm::vec3> vertices;
    vector<GLfloat> vBuffer;

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

out vec3 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    vertexColor = color;
}
)";

const char* fragmentShaderSource = R"(
#version 450 core
in vec3 vertexColor;
out vec4 fragColor;

uniform int selectedObject;
uniform int currentObject; // passado pelo código para identificar o objeto atual

void main() {
    // Se for o objeto selecionado, destacamos a cor (ex: amarelo)
    if (selectedObject == currentObject) {
        fragColor = vec4(1.0, 1.0, 0.0, 1.0); // amarelo vivo
    } else {
        fragColor = vec4(vertexColor, 1.0);
    }
}
)";

void handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
GLuint createShaderProgram();
void configureOpenGL(GLFWwindow* window);

void printInstructions() {
    cout << "===== Controles da Aplicação =====" << endl;
    cout << "TAB           : Selecionar próximo objeto" << endl;
    cout << "ESC           : Fechar aplicação" << endl;
    cout << "X, Y, Z       : Rotacionar objeto selecionado nos eixos X, Y e Z" << endl;
    cout << "W, A, S, D    : Mover objeto selecionado para frente, esquerda, trás e direita" << endl;
    cout << "I, J          : Mover objeto selecionado para cima e para baixo" << endl;
    cout << "[             : Diminuir escala do objeto selecionado" << endl;
    cout << "]             : Aumentar escala do objeto selecionado" << endl;
    cout << "===================================" << endl;
}

int main() {
    if (!glfwInit()) {
        cerr << "Falha ao inicializar GLFW" << endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vivencial 1 - Objetos Múltiplos", nullptr, nullptr);
    if (!window) {
        cerr << "Falha ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }

    configureOpenGL(window);
    printInstructions();

    GLuint shader = createShaderProgram();

    GLint modelLoc = glGetUniformLocation(shader, "model");
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    GLint selectedObjLoc = glGetUniformLocation(shader, "selectedObject");
    GLint currentObjLoc = glGetUniformLocation(shader, "currentObject");

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -8));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

    vector<glm::vec3> positions = {{-2, 0, 0}, {0, 0, 0}, {2, 0, 0}};
    vector<glm::vec3> colors = {
        {1.0f, 0.0f, 0.0f},  // vermelho
        {0.0f, 1.0f, 0.0f},  // verde
        {0.0f, 0.0f, 1.0f}   // azul
    };

    for (size_t i = 0; i < positions.size(); ++i) {
        int nVerts;
        GLuint vao = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVerts, colors[i]);
        if (vao == -1) return -1;

        Object3D obj;
        obj.vao = vao;
        obj.vertexCount = nVerts;
        obj.position = positions[i];
        obj.rotation = glm::vec3(0);
        obj.scale = 1.0f;
        obj.color = colors[i];
        objects.push_back(obj);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1i(selectedObjLoc, selectedObjectIndex);

        for (size_t i = 0; i < objects.size(); ++i) {
            Object3D& obj = objects[i];
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, obj.rotation.x, glm::vec3(1, 0, 0));
            model = glm::rotate(model, obj.rotation.y, glm::vec3(0, 1, 0));
            model = glm::rotate(model, obj.rotation.z, glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(obj.scale));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(currentObjLoc, (int)i);

            glBindVertexArray(obj.vao);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void configureOpenGL(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, handleKeyboard);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD" << endl;
        exit(-1);
    }
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
}

void handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    Object3D& obj = objects[selectedObjectIndex];

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, true); break;
        case GLFW_KEY_TAB: selectedObjectIndex = (selectedObjectIndex + 1) % objects.size(); break;
        case GLFW_KEY_X: obj.rotation.x += glm::radians(15.0f); break;
        case GLFW_KEY_Y: obj.rotation.y += glm::radians(15.0f); break;
        case GLFW_KEY_Z: obj.rotation.z += glm::radians(15.0f); break;
        case GLFW_KEY_W: obj.position.z -= 0.1f; break;
        case GLFW_KEY_S: obj.position.z += 0.1f; break;
        case GLFW_KEY_A: obj.position.x -= 0.1f; break;
        case GLFW_KEY_D: obj.position.x += 0.1f; break;
        case GLFW_KEY_I: obj.position.y += 0.1f; break;
        case GLFW_KEY_J: obj.position.y -= 0.1f; break;
        case GLFW_KEY_LEFT_BRACKET: obj.scale = max(0.1f, obj.scale - 0.1f); break;
        case GLFW_KEY_RIGHT_BRACKET: obj.scale += 0.1f; break;
    }
}

GLuint createShaderProgram() {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertex);

    GLint success;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
        cerr << "Erro no vertex shader:\n" << infoLog << endl;
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
        cerr << "Erro no fragment shader:\n" << infoLog << endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        cerr << "Erro ao linkar shader program:\n" << infoLog << endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}