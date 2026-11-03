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
GLuint colorVBO;

size_t indexCount = 0;
size_t pyramidIndexCount = 0;
int currentModel = 0;
bool Hidden_Surface = 0;
bool solid_wire = 0;

// 회전, 이동 변수
float angleX = 0.0f;
float angleY = 0.0f;
int rotDirX = 0;
int rotDirY = 0;
float posX = 0.0f;
float posY = 0.0f;
float speed = 0.001f;

// 3D 모델 데이터 구조체
struct Model {
    std::vector<float> positions;        // 정점 좌표 (x, y, z)
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
    std::string vertexSource = filetobuf("vertex14.glsl");
    const char* source = vertexSource.c_str();
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &source, NULL);
    glCompileShader(vertexShader);
}
void make_fragmentShaders() {
    std::string fragmentSource = filetobuf("fragment14.glsl");
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

    // 8개 정점에 대한 고유 색상 데이터
    std::vector<float> colors = {
    1.0f, 0.5f, 0.0f,  // v1: 주황색
    1.0f, 0.0f, 0.0f,  // v2: 빨간색
    0.0f, 1.0f, 0.0f,  // v3: 초록색
    0.0f, 0.4f, 1.0f,  // v4: 파란색
    1.0f, 1.0f, 0.0f,  // v5: 노란색
    0.6f, 0.2f, 1.0f,  // v6: 보라색
    1.0f, 0.0f, 1.0f,  // v7: 마젠타
    0.0f, 1.0f, 1.0f   // v8: 시안색
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &colorVBO);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, model.positions.size() * sizeof(float), model.positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);

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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    if (rotDirX == 1) angleX += 0.1f;
    else if (rotDirX == -1) angleX -= 0.1f;

    if (rotDirY == 1) angleY += 0.1f;
    else if (rotDirY == -1) angleY -= 0.1f;

    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    // 축 회전시키기
    glm::mat4 fixedModel = glm::mat4(1.0f);
    fixedModel = glm::scale(fixedModel, glm::vec3(0.5f));
    fixedModel = glm::rotate(fixedModel, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    fixedModel = glm::rotate(fixedModel, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    GLint mvpLoc = glGetUniformLocation(shaderProgramID, "uMVP");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");
    GLint useVertexColorLoc = glGetUniformLocation(shaderProgramID, "uUseVertexColor");

    // 축 회전시킨거에서 회전각도 변경 + 이동까지
    glm::mat4 objModel = fixedModel;
    objModel = glm::translate(objModel, glm::vec3(posX, posY, 0.0f));
    objModel = glm::rotate(objModel, glm::radians(angleX), glm::vec3(1.0f, 0.0f, 0.0f));
    objModel = glm::rotate(objModel, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));

    // 사각뿔 색깔
    glm::vec3 pyramidColors[5] = {
        glm::vec3(0.8f, 0.2f, 1.0f), glm::vec3(1.0f, 0.2f, 0.2f), glm::vec3(1.0f, 1.0f, 0.2f),
        glm::vec3(0.2f, 1.0f, 0.2f), glm::vec3(1.0f, 0.5f, 0.2f)
    };

    // 정육면체 그리기
    if (currentModel == 0) {
        glm::mat4 cubeModel = glm::translate(objModel, glm::vec3(-0.5f, -0.5f, -0.5f));
        glm::mat4 cubeMVP = projection * cubeModel;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(cubeMVP));
        glUniform1i(useVertexColorLoc, 1);

        glBindVertexArray(vao);
        if(solid_wire == 0) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }
    // 사각뿔 그리기
    else if (currentModel == 1) {
        glm::mat4 pyramidMVP = projection * objModel;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(pyramidMVP));
        glUniform1i(useVertexColorLoc, 0);

        glBindVertexArray(pyramidVAO);
        for (int i = 0; i < 4; ++i) {
            glUniform3f(colorLoc, pyramidColors[i].r, pyramidColors[i].g, pyramidColors[i].b);
            uintptr_t offset = i * 3 * sizeof(unsigned int);
            if (solid_wire == 0) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)offset);
        }
        glUniform3f(colorLoc, pyramidColors[4].r, pyramidColors[4].g, pyramidColors[4].b);
        uintptr_t offset = 4 * 3 * sizeof(unsigned int);
        if (solid_wire == 0) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)offset);
    }

    // 좌표축 그리기
    glm::mat4 axisMVP = projection * fixedModel;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(axisMVP));
    glUniform1i(useVertexColorLoc, 0);

    glBindVertexArray(axisVAO);
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); glDrawArrays(GL_LINES, 0, 2);
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f); glDrawArrays(GL_LINES, 2, 2);
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f); glDrawArrays(GL_LINES, 4, 2);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, true);
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_C) {
        currentModel = 0;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_P) {
        currentModel = 1;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_H) {
        Hidden_Surface = !Hidden_Surface;
        if (Hidden_Surface) {
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_W) {
        solid_wire = !solid_wire;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_X) {
        if (mods & GLFW_MOD_SHIFT) {
            if (rotDirX == -1) {
                rotDirX = 0;
            }
            else {
                rotDirX = -1;
            }
        }
        else {
            if (rotDirX == 1) {
                rotDirX = 0;
            }
            else {
                rotDirX = 1;
            }
        }
    }

    if (action == GLFW_PRESS && key == GLFW_KEY_Y) {
        if (mods & GLFW_MOD_SHIFT) {
            if (rotDirY == -1) {
                rotDirY = 0;
            }
            else {
                rotDirY = -1;
            }
        }
        else {
            if (rotDirY == 1) {
                rotDirY = 0;
            }
            else {
                rotDirY = 1;
            }
        }
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_S) {
        angleX = 0.0f;
        angleY = 0.0f;
        rotDirX = 0;
        rotDirY = 0;
        posX = 0.0f;
        posY = 0.0f;
    }
}

void ProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        if (posY <= 1.0f)
            posY += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        if (posY >= -1.0f)
            posY -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        if (posX >= -1.0f)
            posX -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        if (posX <= 1.0f)
            posX += speed;
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
        ProcessInput(window);
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