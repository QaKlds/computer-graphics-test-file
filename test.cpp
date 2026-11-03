#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
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
GLuint vao, vbo, ebo, colorVBO;

bool open_rect = 0;

// -0.5 ~ 0.5 범위를 갖는 정육면체 정점 좌표 (8개)
const float cubeVertices[] = {
    -0.5f, -0.5f, -0.5f, // 0: 좌하단 후면
     0.5f, -0.5f, -0.5f, // 1: 우하단 후면
     0.5f,  0.5f, -0.5f, // 2: 우상단 후면
    -0.5f,  0.5f, -0.5f, // 3: 좌상단 후면
    -0.5f, -0.5f,  0.5f, // 4: 좌하단 전면
     0.5f, -0.5f,  0.5f, // 5: 우하단 전면
     0.5f,  0.5f,  0.5f, // 6: 우상단 전면
    -0.5f,  0.5f,  0.5f  // 7: 좌상단 전면
};

// 정점별 색상 데이터 (8개 정점에 각각 RGB 지정)
const float cubeColors[] = {
    1.0f, 0.0f, 0.0f, // 빨강
    0.0f, 1.0f, 0.0f, // 초록
    0.0f, 0.0f, 1.0f, // 파랑
    1.0f, 1.0f, 0.0f, // 노랑
    1.0f, 0.0f, 1.0f, // 자홍
    0.0f, 1.0f, 1.0f, // 청록
    1.0f, 0.5f, 0.0f, // 주황
    0.5f, 0.5f, 0.5f  // 회색
};

// 6개 면을 구성하는 삼각형 인덱스 (총 36개)
const unsigned int cubeIndices[] = {
    // 후면 (Z -)
    0, 2, 1,  0, 3, 2,
    // 전면 (Z +)
    4, 5, 6,  4, 6, 7,
    // 좌측면 (X -)
    4, 7, 3,  4, 3, 0,
    // 우측면 (X +)
    1, 6, 5,  1, 2, 6,
    // 하단면 (Y -)
    4, 0, 1,  4, 1, 5,
    // 상단면 (Y +)
    3, 7, 6,  3, 6, 2
};

void make_vertexShaders();
void make_fragmentShaders();

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

// 직접 정의한 버퍼 배열 바인딩
void InitBuffer() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &colorVBO);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // 정점 위치 VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 정점 색상 VBO
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeColors), cubeColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // 인덱스 EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void drawScene() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    // 직교 투영 행렬
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    // 정육면체가 원점 (0,0,0)에 오도록 단위 행렬 사용 (이미 데이터가 -0.5 ~ 0.5)
    glm::mat4 model = glm::mat4(1.0f);

    // 입체감을 위해 살짝 회전 적용 (필요시 조정)
    model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 mvp = projection * model;

    GLint mvpLoc = glGetUniformLocation(shaderProgramID, "uMVP");
    GLint useVertexColorLoc = glGetUniformLocation(shaderProgramID, "uUseVertexColor");

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    if (useVertexColorLoc != -1) {
        glUniform1i(useVertexColorLoc, 1);
    }

    glBindVertexArray(vao);
    // 36개 인덱스로 정육면체 전체 렌더링
    for (int i = 1; i < 7; i++) {
        if (i == 2) {

        }
        glDrawElements(GL_TRIANGLES, i * 6, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, true);
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_O) {
        open_rect = !open_rect;
        if (open_rect == 1) {

        }
    }
}

void ProcessInput(GLFWwindow* window) {
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Direct 3D Cube", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { glfwDestroyWindow(window); glfwTerminate(); return -1; }

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    InitBuffer();

    glEnable(GL_DEPTH_TEST);

    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window)) {
        ProcessInput(window);
        drawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &colorVBO);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgramID);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}