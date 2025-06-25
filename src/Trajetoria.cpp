#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

#include <cmath>
#include <algorithm>
#include "Camera.h"

std::string textureFileName = "../assets/tex/pixelWall.png";
float ka = 0.1f, kd = 0.7f, ks = 0.2f, ns = 10.0f;
const GLuint WIDTH = 800, HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f),
              glm::vec3(0.0f, 1.0f, 0.0f),
              -90.0f, 0.0f);

float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

vector<vec3> trajectory;
int currentPointIndex = 0;
float moveSpeed = 1.0f;
vec3 objectPos(0.0f);

void updateTrajectory(float deltaTime) {
    if (trajectory.empty()) return;

    const vec3& target = trajectory[currentPointIndex];
    vec3 diff = target - objectPos;
    float distance = length(diff);

    if (distance < 0.001f) {
        objectPos = target;
        currentPointIndex = (currentPointIndex + 1) % trajectory.size();
        return;
    }

    float step = moveSpeed * deltaTime;

    if (step >= distance) {
        objectPos = target;
        currentPointIndex = (currentPointIndex + 1) % trajectory.size();
    } else {
        vec3 direction = diff / distance;
        objectPos += direction * step;
    }
}


bool loadTrajectoryFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo de trajetória: " << filename << endl;
        return false;
    }
    trajectory.clear();
    float x, y, z;
    while (file >> x >> y >> z) {
        trajectory.emplace_back(x, y, z);
    }
    file.close();
    cout << "Trajetória carregada com " << trajectory.size() << " pontos.\n";
    return true;
}

void processInput(GLFWwindow* window) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        vec3 point = camera.Position + camera.Front * 3.0f;
        trajectory.push_back(point);
        glfwWaitEventsTimeout(0.2);
    }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        ofstream file("trajetoria.txt");
        for (const vec3& p : trajectory) {
            file << p.x << " " << p.y << " " << p.z << "\n";
        }
        file.close();
        cout << "Trajetória salva com " << trajectory.size() << " pontos.\n";
        glfwWaitEventsTimeout(0.2);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

int loadSimpleOBJ(string filePATH, int &nVertices)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<GLfloat> vBuffer;
    std::vector<glm::vec3> normals;

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    std::string mtlFile;
    while (std::getline(arqEntrada, line)) {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v") {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } else if (word == "vt") {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } else if (word == "f") {
            while (ssline >> word) {
                int vi = 0, ti = 0, ni = 0;
                std::replace(word.begin(), word.end(), '/', ' ');
                std::istringstream indices(word);
                indices >> vi >> ti >> ni;
                vi--; ti--; ni--;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                vBuffer.push_back(texCoords[ti].x);
                vBuffer.push_back(texCoords[ti].y);

                vBuffer.push_back(normals[ni].x);
                vBuffer.push_back(normals[ni].y);
                vBuffer.push_back(normals[ni].z);
            }
        } else if (word == "mtllib") {
            ssline >> mtlFile;
        } else if (word == "vn") {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }

    }
    arqEntrada.close();

    if (!mtlFile.empty()) {
        std::ifstream mtlInput(mtlFile.c_str());
        std::string mtlLine;
        while (std::getline(mtlInput, mtlLine)) {
            std::istringstream ssMtl(mtlLine);
            std::string mtlWord;
            ssMtl >> mtlWord;
            if (mtlWord == "map_Kd") {
                ssMtl >> textureFileName;
            } else if (mtlWord == "Ka") {
                ssMtl >> ka;
            } else if (mtlWord == "Kd") {
                ssMtl >> kd;
            } else if (mtlWord == "Ks") {
                ssMtl >> ks;
            } else if (mtlWord == "Ns") {
                ssMtl >> ns;
            }
        }
    }

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 5;

    return VAO;
}

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();
GLuint loadTexture(string filePath, int &width, int &height);

// Vertex Shader
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
layout (location = 2) in vec3 normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragPos = model * vec4(position, 1.0);
    texCoord = texc;
    vNormal = normal;
})";

const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
in vec3 vNormal;
in vec4 fragPos;

uniform sampler2D texBuff;
uniform vec3 lightPos;
uniform vec3 camPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;

out vec4 color;

void main()
{
    vec3 lightColor = vec3(1.0);
    vec3 objectColor = texture(texBuff, texCoord).rgb;

    vec3 ambient = ka * lightColor;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vec3(fragPos));
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = kd * diff * lightColor;

    vec3 R = reflect(-L, N);
    vec3 V = normalize(camPos - vec3(fragPos));
    float spec = pow(max(dot(R, V), 0.0), q);
    vec3 specular = ks * spec * lightColor;

    vec3 result = (ambient + diffuse) * objectColor + specular;
    color = vec4(result, 1.0);
})";

int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Trajetoria - Pedro Fleck", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    int nVertices;
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices);

    int imgWidth, imgHeight;
    GLuint texID = loadTexture(textureFileName, imgWidth, imgHeight);

    loadTrajectoryFromFile("trajetoria.txt");

    vec3 lightPos = vec3(0.6, 1.2, -0.5);
    vec3 camPos = vec3(0.0, 0.0, -3.0);

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glUniform1f(glGetUniformLocation(shaderID, "ka"), ka);
    glUniform1f(glGetUniformLocation(shaderID, "kd"), kd);
    glUniform1f(glGetUniformLocation(shaderID, "ks"), ks);
    glUniform1f(glGetUniformLocation(shaderID, "q"), ns);
    glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(shaderID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
    glActiveTexture(GL_TEXTURE0);

    mat4 projection = ortho(-2.0f, 2.0f,-2.0f, 2.0f,-2.0f, 2.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        updateTrajectory(deltaTime);
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 projection = perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        mat4 view = camera.GetViewMatrix();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, objectPos);
        model = glm::scale(model, glm::vec3(0.2f));

        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, texID);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint loadTexture(string filePath, int &width, int &height)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        if (nrChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture " << filePath << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}
