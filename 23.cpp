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

// 상자 열기 상태 및 애니메이션 각도
bool open_rect = false;
float lidAngle = 0.0f; // 0도 ~ 90도
float view_rotate_origin = 0.0f;
float view_posZ_origin = -4.0f;
float view_posX_origin = 0.0f;
float walkAngle = 0.0f;

// -0.5 ~ 0.5 범위를 갖는 정육면체 정점 좌표 (8개)
// 1. 정점 좌표 변경 (Y값을 -1.0 ~ 1.0으로 확대하여 높이가 긴 직육면체로 설정)
const float cubeVertices[] = {
    -0.7f, -0.5f, -0.7f, // 0: 좌하단 후면
     0.7f, -0.5f, -0.7f, // 1: 우하단 후면
     0.7f,  0.5f, -0.7f, // 2: 우상단 후면
    -0.7f,  0.5f, -0.7f, // 3: 좌상단 후면
    -0.7f, -0.5f,  0.7f, // 4: 좌하단 전면
     0.7f, -0.5f,  0.7f, // 5: 우하단 전면
     0.7f,  0.5f,  0.7f, // 6: 우상단 전면
    -0.7f,  0.5f,  0.7f  // 7: 좌상단 전면
};

// 정점별 색상 데이터 (8개 정점에 각각 RGB 지정)
const float cubeColors[] = {
    1.0f, 0.0f, 0.0f, // 0: 빨강
    0.0f, 1.0f, 0.0f, // 1: 초록
    0.0f, 0.0f, 1.0f, // 2: 파랑
    1.0f, 1.0f, 0.0f, // 3: 노랑
    1.0f, 0.0f, 1.0f, // 4: 자홍 (전면)
    0.0f, 1.0f, 1.0f, // 5: 청록 (전면)
    1.0f, 0.5f, 0.0f, // 6: 주황 (전면)
    0.5f, 0.5f, 0.5f  // 7: 회색 (전면)
};

const unsigned int cubeIndices[] = {
    // 1. 전면 (Z +) [Index 0 ~ 5]
    4, 5, 6,  4, 6, 7,

    // 2. 후면 (Z -)
    0, 2, 1,  0, 3, 2,
    // 3. 좌측면 (X -)
    4, 7, 3,  4, 3, 0,
    // 4. 우측면 (X +)
    1, 6, 5,  1, 2, 6,
    // 5. 하단면 (Y -)
    4, 0, 1,  4, 1, 5,
    // 6. 상단면 (Y +)
    3, 7, 6,  3, 6, 2
};

struct Obstacle {
    glm::vec3 position; // 랜덤 위치 (X, Y, Z)
    glm::vec3 scale;    // 랜덤 크기 (Width, Height, Depth)
    glm::vec3 color;    // 장애물 색상
    float rotate;
}o;

std::vector<Obstacle> obstacles;

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
    std::string fragmentSource = filetobuf("fragment23.glsl");
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

void DrawRobot(const glm::mat4& vp, const glm::mat4& baseModel) {
    GLint mvpLoc = glGetUniformLocation(shaderProgramID, "uMVP");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uFaceColor");

    glBindVertexArray(vao);

    // ----------------------------------------------------
    // 0. 로봇 전체 위치 및 기본 스케일 설정 (상자 바닥 안착)
    // ----------------------------------------------------
    // 상자 바닥(-0.5) 위에서 로봇 발이 닿도록 중심 Y 위치 조정 (약 -0.32f)
    glm::mat4 robotRoot = glm::translate(baseModel, glm::vec3(0.0f, -0.32f, 0.0f));

    // ----------------------------------------------------
    // 1. 몸통 (Body) - 축소 크기: (0.08, 0.12, 0.06)
    // ----------------------------------------------------
    glm::vec3 bodyScale = glm::vec3(0.06f, 0.12f, 0.03f);
    glm::mat4 bodyModel = robotRoot;

    glm::mat4 renderedBodyModel = glm::scale(bodyModel, bodyScale);
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * renderedBodyModel));
    glUniform3f(colorLoc, 0.6f, 0.6f, 0.6f); // 회색
    for (int i = 0; i < 6; ++i) glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));

    // ----------------------------------------------------
    // 2. 머리 (Head) & 코 (Nose)
    // ----------------------------------------------------
    // 머리: 몸통 위 (Y = +0.09)
    glm::mat4 headModel = glm::translate(bodyModel, glm::vec3(0.0f, 0.09f, 0.0f));
    glm::mat4 renderedHeadModel = glm::scale(headModel, glm::vec3(0.05f, 0.05f, 0.05f));
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * renderedHeadModel));
    glUniform3f(colorLoc, 0.8f, 0.8f, 0.8f); // 연회색
    for (int i = 0; i < 6; ++i) glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));

    // 코: 머리 앞면 (Z = +0.03) -> 앞뒤 구분용
    glm::mat4 noseModel = glm::translate(headModel, glm::vec3(0.0f, 0.0f, 0.03f));
    noseModel = glm::scale(noseModel, glm::vec3(0.012f, 0.012f, 0.02f));
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * noseModel));
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // 빨간 코
    for (int i = 0; i < 6; ++i) glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));

    // ----------------------------------------------------
    // 3. 양 팔 (Left / Right Arm) - 서로 다른 색상
    // ----------------------------------------------------
    glm::vec3 armScale = glm::vec3(0.02f, 0.08f, 0.02f);

    // 왼팔
    glm::vec3 leftShoulder = glm::vec3(-0.05f, 0.04f, 0.0f);
    glm::mat4 leftArmModel = bodyModel;
    leftArmModel = glm::translate(leftArmModel, leftShoulder);
    leftArmModel = glm::rotate(leftArmModel, glm::radians(walkAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    leftArmModel = glm::translate(leftArmModel, -leftShoulder);
    leftArmModel = glm::translate(leftArmModel, glm::vec3(-0.05f, 0.0f, 0.0f));

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * glm::scale(leftArmModel, armScale)));
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f); // 초록색 (왼팔)
    for (int i = 0; i < 6; ++i) glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));

    // 오른팔
    glm::vec3 rightShoulder = glm::vec3(0.05f, 0.04f, 0.0f);
    glm::mat4 rightArmModel = bodyModel;
    rightArmModel = glm::translate(rightArmModel, rightShoulder);
    rightArmModel = glm::rotate(rightArmModel, glm::radians(-walkAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rightArmModel = glm::translate(rightArmModel, -rightShoulder);
    rightArmModel = glm::translate(rightArmModel, glm::vec3(0.05f, 0.0f, 0.0f));

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * glm::scale(rightArmModel, armScale)));
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f); // 파란색 (오른팔)
    for (int i = 0; i < 6; ++i) glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));

    // ----------------------------------------------------
    // 4. 양 다리 (Left / Right Leg) - 팔과 교차하여 흔들기
    // ----------------------------------------------------
    glm::vec3 legScale = glm::vec3(0.024f, 0.09f, 0.024f);

    // 왼다리
    glm::vec3 leftHip = glm::vec3(-0.025f, -0.06f, 0.0f);
    glm::mat4 leftLegModel = bodyModel;
    leftLegModel = glm::translate(leftLegModel, leftHip);
    leftLegModel = glm::rotate(leftLegModel, glm::radians(-walkAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    leftLegModel = glm::translate(leftLegModel, -leftHip);
    leftLegModel = glm::translate(leftLegModel, glm::vec3(-0.025f, -0.09f, 0.0f));

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * glm::scale(leftLegModel, legScale)));
    glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // 노란색 (왼다리)
    for (int i = 0; i < 6; ++i) glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));

    // 오른다리
    glm::vec3 rightHip = glm::vec3(0.025f, -0.06f, 0.0f);
    glm::mat4 rightLegModel = bodyModel;
    rightLegModel = glm::translate(rightLegModel, rightHip);
    rightLegModel = glm::rotate(rightLegModel, glm::radians(walkAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rightLegModel = glm::translate(rightLegModel, -rightHip);
    rightLegModel = glm::translate(rightLegModel, glm::vec3(0.025f, -0.09f, 0.0f));

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(vp * glm::scale(rightLegModel, legScale)));
    glUniform3f(colorLoc, 1.0f, 0.5f, 0.0f); // 주황색 (오른다리)
    for (int i = 0; i < 6; ++i) glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(unsigned int)));

    glBindVertexArray(0);
}

void InitObstacles() {
    obstacles.clear();

    // 최대 시도 횟수 (무한 루프 방지)
    const int maxAttempts = 100;

    for (int i = 0; i < 3; ++i) {
        Obstacle obs;
        bool validPosition = false;
        int attempts = 0;

        while (!validPosition && attempts < maxAttempts) {
            attempts++;

            // 1. 크기 및 회전 랜덤 지정
            float sx = ((float)rand() / RAND_MAX) * 0.2f + 0.1f;
            float sy = ((float)rand() / RAND_MAX) * 0.1f + 0.05f;
            float sz = ((float)rand() / RAND_MAX) * 0.2f + 0.01f;

            // 2. 위치 랜덤 지정 (상자 내부 벽 안쪽 범위)
            float rx = ((float)rand() / RAND_MAX) * 1.1f - 0.55f;
            float rz = ((float)rand() / RAND_MAX) * 1.1f - 0.55f;

            float bottomY = -0.49f; // 바닥 Y 좌표 (-0.5f 또는 -0.7f 등 본인 설정값)
            glm::vec3 tempPos = glm::vec3(rx, bottomY + (sy * 0.5f), rz);

            // 3. 기존에 생성된 장애물들과 겹치는지 거리 검사
            bool overlap = false;
            for (const auto& existingObs : obstacles) {
                // XZ 평면상에서 두 장애물 중심 간 거리 계산
                float dist = glm::distance(glm::vec2(tempPos.x, tempPos.z),
                    glm::vec2(existingObs.position.x, existingObs.position.z));

                // 두 장애물 크기(대각선/반지름)를 고려한 최소 안전 거리 설정
                float minDistance = (sx + existingObs.scale.x) * 0.8f + 0.1f;

                if (dist < minDistance) {
                    overlap = true; // 겹침 발생!
                    break;
                }
            }

            // 4. 겹치지 않았다면 최종 확정
            if (!overlap) {
                obs.position = tempPos;
                obs.scale = glm::vec3(sx, sy, sz);
                obs.color = glm::vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
                obs.rotate = ((float)rand() / RAND_MAX) * 360.0f;

                validPosition = true;
            }
        }

        // 겹치지 않는 적절한 위치를 찾았을 때만 리스트에 추가
        if (validPosition) {
            obstacles.push_back(obs);
        }
    }
}

void DrawObstacles(const glm::mat4& vp, const glm::mat4& baseModel) {
    GLint mvpLoc = glGetUniformLocation(shaderProgramID, "uMVP");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uFaceColor");

    glBindVertexArray(vao);

    for (const auto& obs : obstacles) {
        // 1. 모델 행렬 계산 (이동 -> 스케일 변환)
        glm::mat4 obsmodel = glm::mat4(1.0f);
        obsmodel = glm::translate(obsmodel, obs.position);
        obsmodel = glm::rotate(obsmodel, glm::radians(obs.rotate), glm::vec3(0.0f, 1.0f, 0.0f)); // Y축 기준 회전
        obsmodel = glm::scale(obsmodel, obs.scale);

        glm::mat4 finalModel = baseModel * obsmodel;
        glm::mat4 mvp = vp * finalModel;

        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));   
        glUniform3fv(colorLoc, 1, glm::value_ptr(obs.color));

        // 3. 기존 육면체 인덱스(36개) 그리기
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}


void drawScene() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // 시선 방향의 앞면(Front Face)을 Culling

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(view_posX_origin, 0.0f, view_posZ_origin));
    view = glm::rotate(view, glm::radians(view_rotate_origin), glm::vec3(0.0f, 1.0f, 0.0f));

    // 전체 상자의 기본 모델 행렬 (약간 회전하여 입체감 확보)
    glm::mat4 baseModel = glm::mat4(1.0f);
    //baseModel = glm::rotate(baseModel, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 vp = projection * view;

    GLint mvpLoc = glGetUniformLocation(shaderProgramID, "uMVP");
    GLint useVertexColorLoc = glGetUniformLocation(shaderProgramID, "uUseVertexColor");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uFaceColor");

    // 면별 색상 배열
    glm::vec3 colors[6] = {
        glm::vec3(1.0f, 0.0f, 1.0f), // 전면 (자홍)
        glm::vec3(1.0f, 0.0f, 0.0f), // 후면 (빨강)
        glm::vec3(0.0f, 1.0f, 0.0f), // 좌측 (초록)
        glm::vec3(0.0f, 0.0f, 1.0f), // 우측 (파랑)
        glm::vec3(1.0f, 1.0f, 0.0f), // 하단 (노랑)
        glm::vec3(0.0f, 1.0f, 1.0f)  // 상단 (청록)
    };

    if (useVertexColorLoc != -1) {
        glUniform1i(useVertexColorLoc, 1);
    }

    glBindVertexArray(vao);

    // 1. 고정된 나머지 5개 면 렌더링 (인덱스 6부터 30개)
    glm::mat4 bodyMVP = vp * baseModel;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(bodyMVP));
    for (int i = 1; i < 6; ++i) {
        glUniform3fv(colorLoc, 1, glm::value_ptr(colors[i])); // 해당 면 색상 전달
        uintptr_t offset = i * 6 * sizeof(unsigned int);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)offset);
    }

    // 2. 전면(Z+) 뚜껑 렌더링 (힌지 기준 회전 적용)
    // 전면 하단 엣지 중점인 (0.0, -0.5, 0.5)를 회전 힌지 축(Pivot)으로 지정
    glm::vec3 pivot = glm::vec3(0.0f, 0.5f, 0.7f);

    glm::mat4 lidModel = baseModel;
    lidModel = glm::translate(lidModel, pivot);                         // 3. 다시 원래 위치로 복원
    lidModel = glm::rotate(lidModel, glm::radians(lidAngle), glm::vec3(1.0f, 0.0f, 0.0f)); // 2. X축 기준 아래로 회전
    lidModel = glm::translate(lidModel, -pivot);                        // 1. 회전 중심(Pivot)을 원점으로 이동

    glm::mat4 lidMVP = vp * lidModel;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(lidMVP));
    glUniform3fv(colorLoc, 1, glm::value_ptr(colors[0])); // 전면 색상 전달

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

    glDisable(GL_CULL_FACE);

    DrawObstacles(vp, baseModel); // 장애물 그리기
    DrawRobot(vp, baseModel);

    glBindVertexArray(0);


}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, true);
    }
    /*
    if (action == GLFW_PRESS && key == GLFW_KEY_O) {
        open_rect = !open_rect; // O키 누르면 열림/닫힘 토글
    }
    */
}

void ProcessInput(GLFWwindow* window) {
    // 열림/닫힘 상태에 따른 뚜껑 각도 보간 (속도 조절 가능)
    float speed = 0.5f;
    if (open_rect) {
        if (lidAngle > -90.0f) lidAngle -= speed;
        if (lidAngle < -90.0f) lidAngle = -90.0f;
    }
    else {
        if (lidAngle < 0.0f) lidAngle += speed;
        if (lidAngle > 0.0f) lidAngle = 0.0f;
    }

    bool isShiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        if (isShiftPressed) {
            // Shift + Y 누른 경우
            view_rotate_origin += 0.1f;
        }
        else {
            // 그냥 Y 누른 경우
            view_rotate_origin -= 0.1f;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (isShiftPressed) {
            // Shift + Z 누른 경우
            view_posZ_origin -= 0.001f;
        }
        else {
            // 그냥 Z 누른 경우
            view_posZ_origin += 0.001f;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (isShiftPressed) {
            // Shift + X 누른 경우
            view_posX_origin += 0.001f;
        }
        else {
            // 그냥 X 누른 경우
            view_posX_origin -= 0.001f;
        }
    }

    float time = (float)glfwGetTime();
    walkAngle = sin(time * 4.0f) * 30.0f;

}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Openable Cube", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { glfwDestroyWindow(window); glfwTerminate(); return -1; }

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    InitBuffer();
    InitObstacles();

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