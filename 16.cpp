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
bool t_rotate = 0;
bool f_rotate = 0;
bool s_rotate = 0;
bool b_scale = 0;
bool rotate_1 = 0;
bool rotate_2 = 0;
bool rotate_4 = 0;
bool rotate_5 = 0;
bool rotate_6 = 0;


// 회전, 이동 변수
float angleX = 0.0f;
float angleY = 0.0f;
float t_origin = 0.0f;
float f_origin = 0.0f;
float s_origin = 0.0f;
float b_origin = 1.0f;
float origin_1 = 0.0f;
float origin_2 = 0.0f;
float origin_4 = 0.0f;
float origin_5 = 0.0f;
float origin_6_1 = 0.0f;
float origin_6_2 = 0.0f;
float origin_6_3 = 0.0f;
float origin_6_4 = 0.0f;


int f_dir = 1;
int dir_6_1 = -1;
int dir_6_2 = -1;
int dir_6_3= -1;
int dir_6_4= -1;
int b_dir = 1;
int rotDirX = 0;
int rotDirY = 0;
float posX = 0.0f;
float posY = 0.0f;
float speed = 0.001f;

//랜덤 두면 고르는 변수
int randface1 = -1;
int randface2 = -1;

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

    if (t_rotate) {
        t_origin += 0.1f;
    }
    if (f_rotate) {
        float f_speed = 0.1f;

        f_origin += f_speed * f_dir;

        if (f_origin >= 0.0f) {
            f_origin = 0.0f;
            f_dir = -1;
        }
        else if (f_origin <= -90.0f) {
            f_origin = -90.0f;
            f_dir = 1;
        }
    }
    if (b_scale) {
        float b_speed = 0.0005f;

        b_origin += b_speed * b_dir;

        if (b_origin >= 1.0f) {
            b_origin = 1.0f;
            b_dir = -1;
        }
        else if (b_origin <= 0.0f) {
            b_origin = 0.0f;
            b_dir = 1;
        }
    }
    if (s_rotate) {
        s_origin += 0.1f;
    }
    if (rotate_1) {
        origin_1 += 0.1f;
    }
    if (rotate_2) {
        if (origin_2 < 90.0f)
            origin_2 += 0.1f;
    }
    if (rotate_4) {
        origin_4 += 0.1f;
    }
    if (rotate_5) {
        origin_5 += 0.1f;
    }
    if (rotate_6) {
        origin_6_1 += 0.1f;
        origin_6_2 += 0.1f;
        origin_6_3 += 0.1f;
        origin_6_4 += 0.1f;

        float speed_6 = 0.1f;
        origin_6_1 += speed_6 * dir_6_1;
        origin_6_2 += speed_6 * dir_6_2;
        origin_6_3 += speed_6 * dir_6_3;
        origin_6_4 += speed_6 * dir_6_4;

        if (origin_6_1 >= 0.0f) {
            origin_6_1 = 0.0f;
            dir_6_1= 1;
        }
        else if (origin_6_1 <= -90.0f) {
            origin_6_1 = -90.0f;
            dir_6_1 = -1;
        }

        if (origin_6_2 >= 0.0f) {
            origin_6_2 = 0.0f;
            dir_6_2 = 1;
        }
        else if (origin_6_2 <= -90.0f) {
            origin_6_2 = -90.0f;
            dir_6_2 = -1;
        }

        if (origin_6_3 >= 0.0f) {
            origin_6_3 = 0.0f;
            dir_6_3 = 1;
        }
        else if (origin_6_3 <= -90.0f) {
            origin_6_3 = -90.0f;
            dir_6_3 = -1;
        }

        if (origin_6_4 >= 0.0f) {
            origin_6_4 = 0.0f;
            dir_6_4 = 1;
        }
        else if (origin_6_4 <= -90.0f) {
            origin_6_4 = -90.0f;
            dir_6_4 = -1;
        }
    }


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

        if (solid_wire == 0) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } 
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }


        for (int i = 0; i < 6; i++) {
            glm::mat4 faceModel = objModel;

            if (i == 2 && t_rotate == 1) {
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.5f, 0.0f));
                faceModel = glm::rotate(faceModel, glm::radians(t_origin), glm::vec3(0.0f, 0.0f, 1.0f));
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, -0.5f, 0.0f));
            }
            if (i == 5 && f_rotate == 1) {
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.5f, 0.5f));
                faceModel = glm::rotate(faceModel, glm::radians(f_origin), glm::vec3(1.0f, 0.0f, 0.0f));
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, -0.5f, -0.5f));
            }
            if (i == 0 && b_scale == 1) {
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.0f, -0.5f));
                faceModel = glm::scale(faceModel, glm::vec3(b_origin));
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.0f, 0.5f));
            }
            if (i == 1 && s_rotate == 1) {
                faceModel = glm::translate(faceModel, glm::vec3(-0.5f, 0.0f, 0.0f));
                faceModel = glm::rotate(faceModel, glm::radians(s_origin), glm::vec3(1.0f, 0.0f, 0.0f));
                faceModel = glm::translate(faceModel, glm::vec3(0.5f, 0.0f, 0.0f));
            }
            if (i == 3 && s_rotate == 1) {
                faceModel = glm::translate(faceModel, glm::vec3(-0.5f, 0.0f, 0.0f));
                faceModel = glm::rotate(faceModel, glm::radians(s_origin), glm::vec3(1.0f, 0.0f, 0.0f));
                faceModel = glm::translate(faceModel, glm::vec3(0.5f, 0.0f, 0.0f));
            }
            // 여기부터 2면짜리
            if ((i == randface1 || i == randface2) && rotate_1 == 1) {
                if ((randface1 == 0 && randface2 == 1) || (randface1 == 1 && randface2 == 0)) {
                    faceModel = glm::translate(faceModel, glm::vec3(-0.5f, 0.0f, -0.5f));
                    faceModel = glm::rotate(faceModel, glm::radians(origin_1), glm::vec3(0.0f, 1.0f, 0.0f));
                    faceModel = glm::translate(faceModel, glm::vec3(0.5f, 0.0f, 0.5f));
                }
                else if ((randface1 == 1 && randface2 == 5) || (randface1 == 5 && randface2 == 1)) {
                    faceModel = glm::translate(faceModel, glm::vec3(-0.5f, 0.0f, 0.5f));
                    faceModel = glm::rotate(faceModel, glm::radians(origin_1), glm::vec3(0.0f, 1.0f, 0.0f));
                    faceModel = glm::translate(faceModel, glm::vec3(0.5f, 0.0f, -0.5f));
                }
                else if ((randface1 == 5 && randface2 == 3) || (randface1 == 3 && randface2 == 5)) {
                    faceModel = glm::translate(faceModel, glm::vec3(0.5f, 0.0f, 0.5f));
                    faceModel = glm::rotate(faceModel, glm::radians(origin_1), glm::vec3(0.0f, 1.0f, 0.0f));
                    faceModel = glm::translate(faceModel, glm::vec3(-0.5f, 0.0f, -0.5f));
                }
                else if ((randface1 == 3 && randface2 == 0) || (randface1 == 0 && randface2 == 3)) {
                    faceModel = glm::translate(faceModel, glm::vec3(0.5f, 0.0f, -0.5f));
                    faceModel = glm::rotate(faceModel, glm::radians(origin_1), glm::vec3(0.0f, 1.0f, 0.0f));
                    faceModel = glm::translate(faceModel, glm::vec3(-0.5f, 0.0f, 0.5f));
                }
            }
            if ((i == randface1 || i == randface2) && rotate_2 == 1) {
                if ((randface1 == 0 && randface2 == 1) || (randface1 == 1 && randface2 == 0)) {
                    if (i == 0 ) { //반시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                    if (i == 1) { //시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(-origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                }
                else if ((randface1 == 1 && randface2 == 5) || (randface1 == 5 && randface2 == 1)) {
                    if (i == 1) { //반시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                    if (i == 5) { //시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(-origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                }
                else if ((randface1 == 5 && randface2 == 3) || (randface1 == 3 && randface2 == 5)) {
                    if (i == 5) { //반시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                    if (i == 3) { //시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(-origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                }
                else if ((randface1 == 3 && randface2 == 0) || (randface1 == 0 && randface2 == 3)) {
                    if (i == 3) { //반시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                    if (i == 0) { //시계 회전해야함
                        faceModel = glm::rotate(faceModel, glm::radians(-origin_2), glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                }
            } // 옆면이 5 1 3 0
            if (rotate_4 == 1) {
                if (i == 5) {
                    faceModel = glm::rotate(faceModel, glm::radians(origin_4), glm::vec3(0.0f, 1.0f, 0.0f));
                }
                else if (i == 1) {
                    faceModel = glm::rotate(faceModel, glm::radians(origin_4), glm::vec3(0.0f, 1.0f, .0f));
                }
                else if (i == 3) {
                    faceModel = glm::rotate(faceModel, glm::radians(origin_4), glm::vec3(0.0f, 1.0f, 0.0f));
                }
                else if (i == 0) {
                    faceModel = glm::rotate(faceModel, glm::radians(origin_4), glm::vec3(0.0f, 1.0f, 0.0f));
                }
            }
            if (rotate_5 == 1) {
                    if (i == 2) {
                        faceModel = glm::rotate(faceModel, glm::radians(origin_5), glm::vec3(1.0f, 0.0f, 0.0f));
                    }
                    else if (i == 5) {
                        faceModel = glm::rotate(faceModel, glm::radians(origin_5), glm::vec3(1.0f, 0.0f, 0.0f));
                    }
                    else if (i == 0) {
                        faceModel = glm::rotate(faceModel, glm::radians(origin_5), glm::vec3(1.0f, 0.0f, 0.0f));
                    }
                    else if (i == 4) {
                        faceModel = glm::rotate(faceModel, glm::radians(origin_5), glm::vec3(1.0f, 0.0f, 0.0f));
                    }
            }

            faceModel = glm::translate(faceModel, glm::vec3(-0.5f, -0.5f, -0.5f));

            glm::mat4 faceMVP = projection * faceModel;
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(faceMVP));

            uintptr_t offset = i * 6 * sizeof(unsigned int);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)offset);
        }

    }
    // 사각뿔 그리기
    else if (currentModel == 1) {
        glm::mat4 pyramidMVP = projection * objModel;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(pyramidMVP));
        glUniform1i(useVertexColorLoc, 0);

        glBindVertexArray(pyramidVAO);
        if (solid_wire == 0) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        for (int i = 0; i < 5; ++i) {
            glm::mat4 faceModel_t = objModel;

            if (rotate_6 == 1) {
                if (i == 0) { //맨 앞면
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(0.0f, -0.5f, 0.5f));
                    faceModel_t = glm::rotate(faceModel_t, glm::radians(origin_6_2), glm::vec3(1.0f, 0.0f, 0.0f));
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(0.0f, 0.5f, -0.5f));
                }
                else if (i == 1) { //오른쪽 면
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(0.5f, -0.5f, 0.5f));
                    faceModel_t = glm::rotate(faceModel_t, glm::radians(origin_6_3), glm::vec3(0.0f, 0.0f, 1.0f));
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(-0.5f, 0.5f, -0.5f));
                }
                else if (i == 2) { // 뒷면
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(0.0f, -0.5f, -0.5f));
                    faceModel_t = glm::rotate(faceModel_t, glm::radians(origin_6_4), glm::vec3(1.0f, 0.0f, 0.0f));
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(0.0f, 0.5f, 0.5f));
                }
                else if (i == 3) { //왼쪽 면
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(-0.5f, -0.5f, 0.5f));
                    faceModel_t = glm::rotate(faceModel_t, glm::radians(origin_6_1), glm::vec3(0.0f, 0.0f, 1.0f));
                    faceModel_t = glm::translate(faceModel_t, glm::vec3(0.5f, 0.5f, -0.5f));
                }
            }


            glm::mat4 pyramidMVP = projection * faceModel_t;
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(pyramidMVP));

            glUniform3f(colorLoc, pyramidColors[i].r, pyramidColors[i].g, pyramidColors[i].b);
            uintptr_t offset = i * 3 * sizeof(unsigned int);

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
    if (action == GLFW_PRESS && key == GLFW_KEY_P) {
        currentModel = !currentModel;
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
    if (action == GLFW_PRESS && key == GLFW_KEY_R) {
        angleX = 0.0f;
        angleY = 0.0f;
        rotDirX = 0;
        rotDirY = 0;
        posX = 0.0f;
        posY = 0.0f;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_T) {
        t_rotate = !t_rotate;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_F) {
        f_rotate = !f_rotate;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_B) {
        b_scale = !b_scale;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_S) {
        s_rotate = !s_rotate;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_1) {
        rotate_1 = !rotate_1;

        int pair_1[4][2] = { {0, 1}, {1, 5}, {5, 3}, {3, 0} };
        int pair_1_idx = rand() % 4;

        randface1 = pair_1[pair_1_idx][0];
        randface2 = pair_1[pair_1_idx][1];
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_2) {
        rotate_2 = !rotate_2;

        if (!rotate_2) origin_2 = 0.0f;

        int pair_2[4][2] = { {0, 1}, {1, 5}, {5, 3}, {3, 0} };
        int pair_2_idx = rand() % 4;

        randface1 = pair_2[pair_2_idx][0];
        randface2 = pair_2[pair_2_idx][1];
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_4) {
        rotate_4 = !rotate_4;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_5) {
        rotate_5 = !rotate_5;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_6) {
        rotate_6 = !rotate_6;
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
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgramID);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}