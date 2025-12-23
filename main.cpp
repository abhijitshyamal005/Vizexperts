// Enhanced Catmull-ROM Spline with Labels and Wireframe
// Compile: g++ main.cpp -o spline.exe -I./include -L./lib -lfreeglut -lopengl32 -lglu32

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <sstream>
#include <string>

// Texture loader
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Point {
    float x, y;
};

std::vector<Point> controlPoints;
std::vector<Point> splinePoints;
float rotationAngle = 0.0f;
float extrusionWidth = 0.3f;
bool showWireframe = true;
bool showLabels = true;
bool autoRotate = false;
std::string textureFilePath;
GLuint textureId = 0;
float textureRepeat = 4.0f;

// Catmull-ROM spline interpolation
Point catmullRom(const Point& p0, const Point& p1, const Point& p2, const Point& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    Point result;
    result.x = 0.5f * ((2.0f * p1.x) +
                       (-p0.x + p2.x) * t +
                       (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                       (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    
    result.y = 0.5f * ((2.0f * p1.y) +
                       (-p0.y + p2.y) * t +
                       (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                       (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    
    return result;
}

// Load texture from disk using stb_image (supports PNG/JPEG)
GLuint loadTexture(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if (!data) {
        std::cout << "Warning: could not load texture " << filename << " (" << stbi_failure_reason() << ")" << std::endl;
        return 0;
    }

    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    std::cout << "Loaded texture: " << filename << " (" << width << "x" << height << ")" << std::endl;
    return texId;
}

bool readPoints(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        Point p;
        char comma;
        ss >> p.x >> comma >> p.y;
        controlPoints.push_back(p);
    }
    
    std::cout << "Loaded " << controlPoints.size() << " control points" << std::endl;
    return true;
}

void generateSpline() {
    splinePoints.clear();
    
    if (controlPoints.size() < 2) return;
    
    int segments = 40;
    
    for (size_t i = 0; i < controlPoints.size() - 1; i++) {
        Point p0 = (i == 0) ? controlPoints[i] : controlPoints[i-1];
        Point p1 = controlPoints[i];
        Point p2 = controlPoints[i+1];
        Point p3 = (i+2 < controlPoints.size()) ? controlPoints[i+2] : controlPoints[i+1];
        
        for (int j = 0; j < segments; j++) {
            float t = j / (float)segments;
            splinePoints.push_back(catmullRom(p0, p1, p2, p3, t));
        }
    }
    splinePoints.push_back(controlPoints.back());
}

void drawText(const char* text, float x, float y, float z) {
    glRasterPos3f(x, y, z);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void drawControlPoints() {
    glDisable(GL_LIGHTING);
    glPointSize(8.0f);
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    
    glBegin(GL_POINTS);
    for (const auto& p : controlPoints) {
        glVertex3f(p.x, p.y, 0.0f);
    }
    glEnd();
    
    // Draw lines connecting control points
    glLineWidth(1.0f);
    glColor3f(0.3f, 0.5f, 0.7f);
    glBegin(GL_LINE_STRIP);
    for (const auto& p : controlPoints) {
        glVertex3f(p.x, p.y, 0.0f);
    }
    glEnd();
    
    // Label for spline points
    if (showLabels && controlPoints.size() > 0) {
        glColor3f(1.0f, 0.4f, 0.4f);
        drawText("Spline Points", controlPoints[0].x - 2.5f, controlPoints[0].y + 0.8f, 0.0f);
    }
}

void drawSplineCurve() {
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3f(0.5f, 0.7f, 0.9f); // Light blue
    
    glBegin(GL_LINE_STRIP);
    for (const auto& p : splinePoints) {
        glVertex3f(p.x, p.y, 0.0f);
    }
    glEnd();
    
    // Main title
    if (showLabels && splinePoints.size() > 0) {
        glColor3f(1.0f, 0.0f, 0.0f);
        size_t mid = splinePoints.size() / 2;
        drawText("Catmull-ROM Spline", splinePoints[mid].x + 1.0f, splinePoints[mid].y + 2.0f, 0.0f);
    }
}

void drawExtrudedGeometry() {
    if (splinePoints.size() < 2) return;
    
    // Compute arc-length for V texture coords
    float totalLength = 0.0f;
    std::vector<float> cumulativeLen(splinePoints.size(), 0.0f);
    for (size_t i = 1; i < splinePoints.size(); ++i) {
        float dx = splinePoints[i].x - splinePoints[i-1].x;
        float dy = splinePoints[i].y - splinePoints[i-1].y;
        float segLen = std::sqrt(dx*dx + dy*dy);
        totalLength += segLen;
        cumulativeLen[i] = totalLength;
    }
    if (totalLength < 0.0001f) return;
    
    int sides = 16;
    
    // Draw solid geometry
    if (!showWireframe) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (textureId) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
    } else {
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_TEXTURE_2D);
    }
    
    for (size_t i = 0; i < splinePoints.size() - 1; i++) {
        Point curr = splinePoints[i];
        Point next = splinePoints[i+1];
        
        float dx = next.x - curr.x;
        float dy = next.y - curr.y;
        float len = sqrt(dx*dx + dy*dy);
        if (len < 0.001f) continue;
        
        dx /= len;
        dy /= len;
        
        float perpX = -dy;
        float perpY = dx;
        
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= sides; j++) {
            float angle = (j / (float)sides) * 2.0f * M_PI;
            float cosA = cos(angle);
            float sinA = sin(angle);
            
            float offsetX = perpX * cosA * extrusionWidth;
            float offsetY = perpY * cosA * extrusionWidth;
            float offsetZ = sinA * extrusionWidth;
            
            if (!showWireframe) {
                glNormal3f(offsetX / extrusionWidth, offsetY / extrusionWidth, offsetZ / extrusionWidth);
                float t = i / (float)splinePoints.size();
                glColor3f(0.3f + 0.5f * t, 0.4f, 0.9f - 0.4f * t);
                float u = j / (float)sides; // around circumference
                float v = (cumulativeLen[i] / totalLength) * textureRepeat;
                if (textureId) glTexCoord2f(u, v);
            } else {
                glColor3f(0.7f, 0.7f, 0.8f);
            }
            
            glVertex3f(curr.x + offsetX, curr.y + offsetY, offsetZ);
            glVertex3f(next.x + offsetX, next.y + offsetY, offsetZ);
        }
        glEnd();
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // Label for extruded geometry
    if (showLabels && splinePoints.size() > 10) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 0.0f, 0.0f);
        Point p = splinePoints[10];
        drawText("Extruded Geometry", p.x - 3.0f, p.y - 1.5f, 0.0f);
        
        // Width label
        glColor3f(1.0f, 0.0f, 0.0f);
        drawText("Width", controlPoints[0].x - 1.0f, controlPoints[0].y - 1.0f, 0.0f);
        
        // Draw width indicator arrows
        glLineWidth(2.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        Point p0 = controlPoints[0];
        glBegin(GL_LINES);
        glVertex3f(p0.x, p0.y, -extrusionWidth);
        glVertex3f(p0.x, p0.y, extrusionWidth);
        glEnd();
        
        // Arrow heads
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex3f(p0.x, p0.y, -extrusionWidth);
        glVertex3f(p0.x, p0.y, extrusionWidth);
        glEnd();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Camera position - slightly adjusted for better view
    gluLookAt(8.0, 8.0, 12.0,
              3.5, 4.5, 0.0,
              0.0, 0.0, 1.0);
    
    glRotatef(rotationAngle, 0.0f, 0.0f, 1.0f);
    
    drawExtrudedGeometry();
    drawSplineCurve();
    drawControlPoints();
    
    glutSwapBuffers();
}

void timer(int value) {
    if (autoRotate) {
        rotationAngle += 0.5f;
        if (rotationAngle > 360.0f) rotationAngle -= 360.0f;
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'w': case 'W':
            extrusionWidth += 0.05f;
            std::cout << "Width: " << extrusionWidth << std::endl;
            break;
        case 's': case 'S':
            extrusionWidth -= 0.05f;
            if (extrusionWidth < 0.1f) extrusionWidth = 0.1f;
            std::cout << "Width: " << extrusionWidth << std::endl;
            break;
        case 'f': case 'F':
            showWireframe = !showWireframe;
            std::cout << "Wireframe: " << (showWireframe ? "ON" : "OFF") << std::endl;
            break;
        case 'l': case 'L':
            showLabels = !showLabels;
            std::cout << "Labels: " << (showLabels ? "ON" : "OFF") << std::endl;
            break;
        case 'r': case 'R':
            autoRotate = !autoRotate;
            std::cout << "Auto Rotate: " << (autoRotate ? "ON" : "OFF") << std::endl;
            break;
        case ' ':
            rotationAngle = 0.0f;
            std::cout << "Reset rotation" << std::endl;
            break;
        case 27: // ESC
            exit(0);
            break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_LEFT:
            rotationAngle -= 5.0f;
            break;
        case GLUT_KEY_RIGHT:
            rotationAngle += 5.0f;
            break;
    }
    glutPostRedisplay();
}

void initOpenGL() {
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    
    GLfloat light_position[] = { 10.0f, 10.0f, 10.0f, 1.0f };
    GLfloat light_ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glLineWidth(1.5f);
}

int main(int argc, char** argv) {
    std::string pointsFile = "Assignment Points.txt";
    
    if (argc > 1) pointsFile = argv[1];
    if (argc > 2) extrusionWidth = std::stof(argv[2]);
    
    std::cout << "=== Catmull-ROM Spline Extrusion ===" << std::endl;
    std::cout << "Loading points from: " << pointsFile << std::endl;
    
    if (!readPoints(pointsFile)) {
        std::cout << "Failed to read points file. Creating default points..." << std::endl;
        controlPoints = {{0,0}, {1,1}, {2,3}, {5,1}, {7,8}};
    }
    
    generateSpline();
    
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "  W/S - Increase/Decrease width" << std::endl;
    std::cout << "  F - Toggle wireframe" << std::endl;
    std::cout << "  L - Toggle labels" << std::endl;
    std::cout << "  R - Toggle auto-rotation" << std::endl;
    std::cout << "  SPACE - Reset rotation" << std::endl;
    std::cout << "  Arrow Keys - Manual rotation" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "\nInitial width: " << extrusionWidth << std::endl;
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1400, 900);
    glutCreateWindow("Catmull-ROM Spline Extrusion");
    
    initOpenGL();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, timer, 0);
    
    std::cout << "\n=== Window Created Successfully ===" << std::endl;
    
    glutMainLoop();
    
    return 0;
}