//=============================================================================================
// Mintaprogram: Zšld h‡romszšg. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!!
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Adam Zsombor
// Neptun : X079FB
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

const char* const vertexSource = R"(
    #version 330                // Shader 3.3
    precision highp float;        // normal floats, makes no difference on desktop computers

    uniform mat4 MVP;            // uniform variable, the Model-View-Projection transformation matrix
    layout(location = 0) in vec2 vp;    // Varying input: vp = vertex position is expected in attrib array 0

    void main() {
        gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;        // transform vp from modeling space to normalized device space
    }
)";

const char* const fragmentSource = R"(
    #version 330            // Shader 3.3
    precision highp float;    // normal floats, makes no difference on desktop computers
    
    uniform vec3 color;        // uniform variable, the color of the primitive
    out vec4 outColor;        // computed color of the current pixel

    void main() {
        outColor = vec4(color, 1);    // computed color is the color of the primitive
    }
)";


GPUProgram gpuProgram;

enum Mode {
    POINTE,
    LINE,
    MOVE,
    INTERSECT
};

class Object {
    unsigned int vao, vbo;
    std::vector<vec3> vtx;

public:
    Object() {
        glGenVertexArrays(1, &vao); glBindVertexArray(vao);
        glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    std::vector<vec3>& getVtx() { return vtx; }
    
    void resetVtx() { vtx.clear(); }
    
    void updateGPU() {
        glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
    }

    void Draw(int type, vec3 color) {
        if (vtx.size() > 0) {
            glBindVertexArray(vao);
            gpuProgram.setUniform(color, "color");
            glDrawArrays(type, 0, (int)vtx.size());
        }
    }
};


class PointCollection {
public:
    vec3 red;
    Object* points;

    PointCollection() {
        red = vec3(1.0f, 0.0f, 0.0f);
        points = new Object();
    }

    void addPoint(vec3 p) {
        if (p.x >= -1 && p.y >= -1 && p.x <= 1 && p.y <= 1) {
            points->getVtx().push_back(p);
            printf("Point %0.1f, %0.1f added\n", p.x, p.y);
            points->updateGPU();
        }
    }

    int searchPoint(vec3 p) {
        for (int i = 0; i < (int)points->getVtx().size(); i++) {
            if (length(p - points->getVtx()[i]) <= 0.05f) {
                return i;
            }
        }
        return -1;
    }

    void drawPoints() {
        points->Draw(GL_POINTS, red);
    }
};

class Line {
public:
    vec3 start, end, direction;
    float a, b, c;

    Line(vec3 start, vec3 end, int szkl = 0) : start(start), end(end) {
        direction = end - start;
        
        a = -direction.y;
        b = direction.x;
        c = -dot(vec2(a, b), vec2(start.x, start.y));

        if (szkl == 1) {
            printf("Line added\n");
            printf("\tImplicit: %0.1f x + %0.1f y + %0.1f = 0\n", a, b, c);
            printf("\tParametric: r<t> = <%0.1f, %0.1f> + <%0.1f, %0.1f>t\n", start.x, start.y, direction.x, direction.y);
        }
    }

    vec3 intersect(Line target) {
        float denominator = direction.x * target.direction.y - direction.y * target.direction.x;

        if (denominator == 0) {
            return vec3(-2, -2, 1);
        }

        float t = ((target.start.x - start.x) * target.direction.y - (target.start.y - start.y) * target.direction.x) / denominator;

        vec3 intersection = start + direction * t;
        return intersection;
    }

    int onLine(vec3 p) {
        if ((fabs(a * p.x + b * p.y + c) / sqrt(a * a + b * b)) <= 0.01f) {
            return 1;
        }
        return 0;
    }

    void edging() {
        vec3 p1 = vec3(-1, -1, 1);
        vec3 p2 = vec3(-1, 1, 1);
        vec3 p3 = vec3(1, -1, 1);
        vec3 p4 = vec3(1, 1, 1);

        Line left = Line(p1, p2);
        Line up = Line(p2, p4);
        Line right = Line(p4, p3);
        Line down = Line(p3, p1);

        std::vector<vec3> array;

        vec3 edge = this->intersect(left);
        if (edge.y <= 1 && edge.y >= -1) {
            array.push_back(edge);
        }

        edge = this->intersect(right);
        if (edge.y <= 1 && edge.y >= -1) {
            array.push_back(edge);
        }

        edge = this->intersect(up);
        if (edge.x <= 1 && edge.x >= -1) {
            array.push_back(edge);
        }

        edge = this->intersect(down);
        if (edge.x <= 1 && edge.x >= -1) {
            array.push_back(edge);
        }

        start = array[0];
        end = array[1];
    }
    
    void moveToNewPoint(vec3 newPoint) {
        
        vec3 midPoint = (start + end) * 0.5f;
        vec3 displacement = newPoint - midPoint;

        start = start + displacement;
        end = end + displacement;
        
        direction = end - start;

        a = -direction.y;
        b = direction.x;
        c = -dot(vec2(a, b), vec2(start.x, start.y));
        
        this->edging();
    }
};

class LineCollection {
public:
    vec3 cyan;
    Object* lines;
    std::vector<Line> lines_tomb;

    LineCollection() {
        cyan = vec3(0.0f, 1.0f, 1.0f);
        lines = new Object();
    }

    void addLine(Line l) {
        lines_tomb.push_back(l);
        lines->getVtx().push_back(l.start);
        lines->getVtx().push_back(l.end);
        lines->updateGPU();
    }

    int nearbyLine(vec3 p) {
        for (int i = 0; i < (int)lines_tomb.size(); i++) {
            if (lines_tomb[i].onLine(p) == 1) {
                return i;
            }
        }
        return -1;
    }

    void drawLines() {
        lines->Draw(GL_LINES, cyan);
    }
};

Mode mode;
PointCollection* pointColl;
LineCollection* lineColl;
std::vector<vec3> pressedPoint;
std::vector<Line> pressedLine;
Line* selectedLine;
int mouseDragging = 0;

void recalculateLines() {
    lineColl->lines->resetVtx();
    for (int i = 0; i < (int)lineColl->lines_tomb.size(); i++) {
        lineColl->lines->getVtx().push_back(lineColl->lines_tomb[i].start);
        lineColl->lines->getVtx().push_back(lineColl->lines_tomb[i].end);
    }
    lineColl->lines->updateGPU();
}

void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);
    glPointSize(10.0f);
    glLineWidth(3.0f);
    gpuProgram.create(vertexSource, fragmentSource, "outColor");

    pointColl = new PointCollection();
    lineColl = new LineCollection();
}

void onDisplay() {
    glClearColor(0.5f, 0.5f, 0.5f, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    float MVPtransf[4][4] = { 1, 0, 0, 0,
                              0, 1, 0, 0,
                              0, 0, 1, 0,
                              0, 0, 0, 1 };

    int location = glGetUniformLocation(gpuProgram.getId(), "MVP");
    glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);

    lineColl->drawLines();
    pointColl->drawPoints();
    
    glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'p') {
        printf("Define points\n");
        mode = POINTE;
        selectedLine = NULL;
        pressedPoint.clear();
        pressedLine.clear();
    }
    else if (key == 'l') {
        printf("Define lines\n");
        mode = LINE;
        selectedLine = NULL;
        pressedPoint.clear();
        pressedLine.clear();
    }
    else if (key == 'm') {
        printf("Move\n");
        mode = MOVE;
        selectedLine = NULL;
        pressedPoint.clear();
        pressedLine.clear();
    }
    else if (key == 'i') {
        printf("Intersect\n");
        mode = INTERSECT;
        selectedLine = NULL;
        pressedPoint.clear();
        pressedLine.clear();
    }
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

void onMouseMotion(int pX, int pY) {
    float cX = 2.0f * pX / windowWidth - 1;
    float cY = 1.0f - 2.0f * pY / windowHeight;
    if (selectedLine != NULL) {
        selectedLine->moveToNewPoint(vec3(cX, cY, 0));
        recalculateLines();
        glutPostRedisplay();
    }
}

void onMouse(int button, int state, int pX, int pY) {
    float cX = 2.0f * pX / windowWidth - 1;
    float cY = 1.0f - 2.0f * pY / windowHeight;

    vec3 newPoint(cX, cY, 1.0f);

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        mouseDragging = 1;
        if (mode == POINTE) {
            pointColl->addPoint(newPoint);
            glutPostRedisplay();
        } else if (mode == LINE) {
            int searched = pointColl->searchPoint(newPoint);
            if (searched != -1) {
                pressedPoint.push_back(pointColl->points->getVtx()[searched]);
            }

            if (pressedPoint.size() == 2) {
                if (pressedPoint[0].x != pressedPoint[1].x || pressedPoint[0].y != pressedPoint[1].y) {
                    Line newLine = Line(pressedPoint[0], pressedPoint[1], 1);
                    newLine.edging();
                    pressedPoint.clear();
                    lineColl->addLine(newLine);
                    glutPostRedisplay();
                }
                else {
                    pressedPoint.pop_back();
                }
            }
        } else if (mode == INTERSECT) {
            int nearby = lineColl->nearbyLine(newPoint);
            if (nearby != -1) {
                pressedLine.push_back(lineColl->lines_tomb[nearby]);
            }

            if (pressedLine.size() == 2) {
                vec3 intersection = pressedLine[0].intersect(pressedLine[1]);
                if (intersection.x != -2) {
                    pressedLine.clear();
                    pointColl->addPoint(intersection);
                    glutPostRedisplay();
                }
                else {
                    pressedLine.pop_back();
                }
            }
        } else if (mode == MOVE) {
            if (selectedLine == NULL) {
                int nearby = lineColl->nearbyLine(newPoint);
                if (nearby != -1) {
                    selectedLine = &lineColl->lines_tomb[nearby];
                }
            }
        }
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        selectedLine = NULL;
        mouseDragging = 0;
        glutPostRedisplay();
    }
}

void onIdle() {
}
