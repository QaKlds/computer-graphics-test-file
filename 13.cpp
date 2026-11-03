#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <time.h>
#include <stdlib.h>

#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>

std::string filetobuf(const char* file)
{
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) return "";
    std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    shaderFile.close();
    return source;
}


// Window 크기
GLint width = 900, height = 900;

// OpenGL 객체 변수
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

GLuint vao, vbo, ebo;
GLuint axisVAO, axisVBO;
GLuint pyramidVAO, pyramidVBO, pyramidEBO;
size_t indexCount = 0;
size_t pyramidIndexCount = 0;
int currentModel = 0;
int selectedFace = -1;
int randface1 = -1;
int randface2 = -1;

struct Model {
    std::vector<float> positions;       // 정점 좌표 (x, y, z)
    std::vector<unsigned int> indices;  // 면 인덱스
};

void make_vertexShaders();
void make_fragmentShaders();

// OBJ 파일 로더
bool loadOBJ(const std::string& path, Model& model) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[오류] OBJ 파일을 찾을 수 없습니다: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            model.positions.push_back(x);
            model.positions.push_back(y);
            model.positions.push_back(z);
        }
        else if (type == "f") {
            std::string vStr;
            for (int i = 0; i < 3; ++i) {
                ss >> vStr;
                size_t pos = vStr.find('/');
                int index = 0;
                if (pos != std::string::npos) {
                    index = std::stoi(vStr.substr(0, pos));
                }
                else {
                    index = std::stoi(vStr);
                }
                model.indices.push_back(static_cast<unsigned int>(index - 1));
            }
        }
    }
    file.close();
    return true;
}

// 셰이더 컴파일

void make_vertexShaders() {
    std::string vertexSource = filetobuf("vertex13.glsl");
    const char* source = vertexSource.c_str();
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &source, NULL);
    glCompileShader(vertexShader);
}
void make_fragmentShaders() {
    std::string fragmentSource = filetobuf("fragment13.glsl");
    const char* source = fragmentSource.c_str();
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &source, NULL);
    glCompileShader(fragmentShader);
}

GLuint make_shaderProgram() {
    GLuint shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderID);
    return shaderID;
}

// 버퍼 (VAO, VBO, EBO) 바인딩
void InitBuffer(const Model& model) {
    indexCount = model.indices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, model.positions.size() * sizeof(float), model.positions.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void InitPyramidBuffer(const Model& model) {
    pyramidIndexCount = model.indices.size();

    glGenVertexArrays(1, &pyramidVAO);
    glGenBuffers(1, &pyramidVBO);
    glGenBuffers(1, &pyramidEBO);

    glBindVertexArray(pyramidVAO);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, model.positions.size() * sizeof(float), model.positions.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pyramidEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void InitAxisBuffer() {
    float axisVertices[] = {
        // X축 (선분 1)
        -1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        // Y축 (선분 2)
        0.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        // Z축 (선분 3)
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);

    glBindVertexArray(axisVAO);

    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void drawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.5f));
    model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 mvp = projection * model;

    GLint mvpLoc = glGetUniformLocation(shaderProgramID, "uMVP");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    // 좌표축 그리기
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(axisVAO);

    // X축 (빨강)
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Y축 (초록)
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f);
    glDrawArrays(GL_LINES, 2, 2);

    // Z축 (파랑)
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 4, 2);

    // 큐브 그리기
    glm::mat4 cubeModel = glm::translate(model, glm::vec3(-0.5f, -0.5f, -0.5f));

    glm::mat4 cubeMVP = projection * cubeModel;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(cubeMVP));

    glm::vec3 faceColors[6] = {
        glm::vec3(1.0f, 0.2f, 0.2f), // 1번 면: 빨강
        glm::vec3(0.2f, 1.0f, 0.2f), // 2번 면: 초록
        glm::vec3(0.2f, 0.2f, 1.0f), // 3번 면: 파랑
        glm::vec3(1.0f, 1.0f, 0.2f), // 4번 면: 노랑
        glm::vec3(1.0f, 0.5f, 0.2f), // 5번 면: 주황
        glm::vec3(0.8f, 0.2f, 1.0f)  // 6번 면: 보라
    };

    glm::vec3 pyramidColors[5] = {
        glm::vec3(0.8f, 0.2f, 1.0f), // 옆면 1: 보라
        glm::vec3(1.0f, 0.2f, 0.2f), // 옆면 2: 빨강
        glm::vec3(1.0f, 1.0f, 0.2f), // 옆면 3: 노랑
        glm::vec3(0.2f, 1.0f, 0.2f), // 옆면 4: 초록
        glm::vec3(1.0f, 0.5f, 0.2f)  // 밑면: 주황
    };

    if (currentModel == 0) {
        // 정육면체
        glm::mat4 cubeModel = glm::translate(model, glm::vec3(-0.5f, -0.5f, -0.5f));
        glm::mat4 cubeMVP = projection * cubeModel;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(cubeMVP));

        glBindVertexArray(vao);
        for (int i = 0; i < 6; ++i) {
            if (selectedFace == -1 || selectedFace == i || (selectedFace == -2 && (i == randface1 || i == randface2))) {
                glUniform3f(colorLoc, faceColors[i].r, faceColors[i].g, faceColors[i].b);
                uintptr_t offset = i * 6 * sizeof(unsigned int);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)offset);
            }
        }
    }
    else if (currentModel == 1) {
        // 사각뿔
        glm::mat4 pyramidMVP = projection * model;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(pyramidMVP));

        glBindVertexArray(pyramidVAO);

        // 옆면 그리기
        for (int i = 0; i < 4; ++i) {
            // 선택된 면만 그림
            if (selectedFace == -1 || selectedFace == i || (selectedFace == -2 && (i == randface1 || i == randface2))) {
                glUniform3f(colorLoc, pyramidColors[i].r, pyramidColors[i].g, pyramidColors[i].b);
                uintptr_t offset = i * 3 * sizeof(unsigned int);
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)offset);
            }
        }

        // 밑면 그리기
        if (selectedFace == -1 || selectedFace == 4 || (selectedFace == -2 && (4 == randface1 || 4 == randface2))) {
            glUniform3f(colorLoc, pyramidColors[4].r, pyramidColors[4].g, pyramidColors[4].b);
            uintptr_t offset = 4 * 3 * sizeof(unsigned int);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)offset);
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, true);
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_1) {
        currentModel = 0;
        selectedFace = 2;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_2) {
        currentModel = 0;
        selectedFace = 0;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_3) {
        currentModel = 0;
        selectedFace = 1;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_4) {
        currentModel = 0;
        selectedFace = 5;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_5) {
        currentModel = 0;
        selectedFace = 3;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_6) {
        currentModel = 0;
        selectedFace = 4;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_7) {
        currentModel = 1;
        selectedFace = 0;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_8) {
        currentModel = 1;
        selectedFace = 1;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_9) {
        currentModel = 1;
        selectedFace = 2;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_0) {
        currentModel = 1;
        selectedFace = 3;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_A) {
        currentModel = 0;
        selectedFace = -1;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_S) {
        currentModel = 1;
        selectedFace = -1;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_C) {
        currentModel = 0;
        selectedFace = -2;

        randface1 = rand() % 6;
        do {
            randface2 = rand() % 6;
        } while (randface2 == randface1);
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_T) {
        currentModel = 1;
        selectedFace = -2;

        randface1 = rand() % 4;
        randface2 = 4;
    }

}


int main() {
    srand(time(NULL));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Static 3D Cube Viewer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { glfwDestroyWindow(window); glfwTerminate(); return -1; }

    glEnable(GL_DEPTH_TEST);

    Model cubeModel;
    if (!loadOBJ("cube.obj", cubeModel)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    Model pyramidModel;
    if (!loadOBJ("pyramid.obj", pyramidModel)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    InitBuffer(cubeModel);
    InitPyramidBuffer(pyramidModel);
    InitAxisBuffer();

    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window)) {
        drawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgramID);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}