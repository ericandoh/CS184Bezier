#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <time.h>
#include <math.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <float.h>
#include <vector>

#define PI 3.14159265  // Should be used from mathlib

inline float sqr(float x) { return x*x; }


using namespace std;

/*
//****************************************************

Some side notes

OPENGL 3d rendering
https://www3.ntu.edu.sg/home/ehchua/programming/opengl/CG_Examples.html

IO Stuff
http://www.cplusplus.com/doc/tutorial/files/

//****************************************************
*/

//****************************************************
// Some Classes
//****************************************************

class Viewport;

class Viewport {
  public:
    int w, h; // width and height
};


//in classes
struct vec3 {
  float x, y, z;
};
typedef struct vec3 vec3;

struct color {
  float r, g, b;
};
typedef struct color color;

struct point {
  vec3 pos;
  vec3 norm;
};
typedef struct point point;

struct triangle {
  point a, b, c;
  color* color;
};
typedef struct triangle triangle;

struct curve {
  vec3 points[4];
};
typedef struct curve curve;

struct patch {
  curve curves[4];
};
typedef struct patch patch;

//****************************************************
// Global Variables
//****************************************************

Viewport  viewport;

//step size, for uniform tessellation
float step = 0.1f;
//lambda (max error), for adaptive tessellation
float lambda = 0.1f;
//max subdivision depth, for adaptive tessellation
int max_subdivisions = 7;

//the triangles to render
triangle** triangles;
int* triangle_counts;

color* blue;
color* red;

int triangle_count = 0;
int patch_count = 0;

bool isSmooth = true;
bool isWireframe = false;

float vdistance = 6.0f;
float angle = 0.0f;
float angle_change = 2*PI / 30.0f;

std::vector<triangle> adaptiveTriangleList;

void myDisplay();

void subdivideAdaptiveTriangle(patch* patch, float u1x, float u1y, float u2x, float u2y, float u3x, float u3y, int depth);

bool adaptiveTest(patch* patch, float xave, float yave, vec3* v1, vec3* v2);

//****************************************************
// Simple init function
//****************************************************
void initScene() {
  // 3D setup - shamelessly stolen from https://www3.ntu.edu.sg/home/ehchua/programming/opengl/CG_Examples.html
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClearDepth(1.0f);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  //glDepthFunc(GL_LESS);
  glShadeModel(GL_SMOOTH);

  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  //setup materials
  GLfloat mat_diffuse[] = { 1.0, 0.0, 0.0 };
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0 };
  GLfloat mat_shininess[] = { 50.0 };
  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

  //setup lights
  GLfloat ambientLight[] = {0.1, 0.1, 0.1};
  GLfloat diffuseLight[] = {1.0, 1.0, 1.0};
  GLfloat specularLight[] = {1.0, 1.0, 1.0};
  GLfloat light_position[] = { -3.0, 3.0, 3.0, 0.0 };

  glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
}


void cleanup() {
  free(blue);
  free(triangles);
}

//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
  viewport.w = w;
  viewport.h = h;

  if (h == 0) {
    h = 1;
  }
  GLfloat aspect = (GLfloat)w / (GLfloat)h;

  glViewport (0,0,viewport.w,viewport.h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(45.0f, aspect, 0.1f, 100.0f);
}

void myKeyPressed(unsigned char key, int x, int y) {
  if(key == 32) {
    cleanup();
    exit(0);
  }
  else if (key == 's') {
    isSmooth = !isSmooth;
    if (isSmooth) {
      glShadeModel(GL_SMOOTH);
    }
    else {
      glShadeModel(GL_FLAT);
    }
  }
  else if (key == 'w') {
    isWireframe = !isWireframe;
    if (isWireframe) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }
  myDisplay();
}


void mySpecialInput(int key, int x, int y) {
  switch(key) {
    case GLUT_KEY_UP:
      vdistance -= 1;
      if (vdistance < 0)
        vdistance = 0;
      break;
    case GLUT_KEY_DOWN:
      vdistance += 1;
      break;
    case GLUT_KEY_LEFT:
      angle = fmod((angle + angle_change), 2*PI);
      break;
    case GLUT_KEY_RIGHT:
      angle = fmod((angle - angle_change), 2*PI);
      break;
  }
  myDisplay();
}

//****************************************************
// A routine to set a pixel by drawing a GL point.  This is not a
// general purpose routine as it assumes a lot of stuff specific to
// this example.
//****************************************************

void setPixel(int x, int y, GLfloat r, GLfloat g, GLfloat b) {
  glColor3f(r, g, b);
  glVertex2f(x + 0.5, y + 0.5);   // The 0.5 is to target pixel
  // centers 
  // Note: Need to check for gap
  // bug on inst machines.
}

//****************************************************
// Vector Manipulation Methods
//****************************************************

float dotProduct(vec3 *a, vec3 *b) {
  return a->x * b->x + a->y * b->y + a->z*b->z;
}

void normalize(vec3* vec) {
  float mag = sqrt(pow(vec->x, 2) + pow(vec->y, 2) + pow(vec->z, 2));
  vec->x = vec->x / mag;
  vec->y = vec->y / mag;
  vec->z = vec->z / mag;
}

void scale(vec3 *dest, vec3 *vec, float scale) {
  dest->x = vec->x * scale;
  dest->y = vec->y * scale;
  dest->z = vec->z * scale;
}

void subtract(vec3 *dest, vec3 *a, vec3 *b) {
  dest->x = a->x - b->x;
  dest->y = a->y - b->y;
  dest->z = a->z - b->z;
}

void add(vec3 *dest, vec3 *a, vec3 *b) {
  dest->x = a->x + b->x;
  dest->y = a->y + b->y;
  dest->z = a->z + b->z;
}

void set(vec3* dest, vec3* src) {
  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;
}

void crossProduct(vec3* dest, vec3* first, vec3* second) {
  dest->x = first->y * second->z - first->z * second->y;
  dest->y = first->z * second->x - first->x * second->z;
  dest->z = first->x * second->y - first->y * second->x;
}

float magnitude(vec3* a) {
  return sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
}

void bezCurveInterp(vec3* p, vec3* dPdu, curve* curve, float u) {
  vec3 a;
  vec3 b;
  vec3 c;
  vec3 d;
  vec3 e;
  vec3 temp1;
  vec3 temp2;
  // get vec3 A
  scale(&temp1, &(curve->points[0]), 1.0f - u);
  scale(&temp2, &(curve->points[1]), u);
  add(&a, &temp1, &temp2);
  // get vec3 B
  scale(&temp1, &(curve->points[1]), 1.0f - u);
  scale(&temp2, &(curve->points[2]), u);
  add(&b, &temp1, &temp2);
  // get vec3 C
  scale(&temp1, &(curve->points[2]), 1.0f - u);
  scale(&temp2, &(curve->points[3]), u);
  add(&c, &temp1, &temp2);
  // get vec3 D
  scale(&temp1, &a, 1.0f - u);
  scale(&temp2, &b, u);
  add(&d, &temp1, &temp2);
  // get vec3 E
  scale(&temp1, &b, 1.0f - u);
  scale(&temp2, &c, u);
  add(&e, &temp1, &temp2);
  // get p
  scale(&temp1, &d, 1.0f - u);
  scale(&temp2, &e, u);
  add(p, &temp1, &temp2);
  // get dPdu
  subtract(&temp1, &e, &d);
  scale(dPdu, &temp1, 3.0f);
}

void bezPatchInterp(point* p, patch* patch, float u, float v) {
  curve vcurve;
  curve ucurve;
  vec3 temp;
  curve tempCurve;
  for(int i = 0; i <= 3; i++) {
    bezCurveInterp(&(vcurve.points[i]), &temp, &(patch->curves[i]), u);
    for(int j = 0; j <= 3; j++) {
      set(&(tempCurve.points[j]), &(patch->curves[j].points[i]));
    }
    bezCurveInterp(&(ucurve.points[i]), &temp, &tempCurve, v);
  }
  vec3 dPdv;
  vec3 dPdu;
  // surface derivatives
  bezCurveInterp(&(p->pos), &dPdv, &vcurve, v);
  bezCurveInterp(&(p->pos), &dPdu, &ucurve, u);
  
  crossProduct(&(p->norm), &dPdu, &dPdv);
  normalize(&(p->norm));
}

triangle* subdivideUniform(patch* patch, float step, color* color) {
  //performs uniform subdivision
  //temp variables u, v
  float u, v;

  //round a small bit to account for rounding error
  //number of subdivisions
  int numdiv = (int) (ceil(1.000f / step));
  numdiv += 1;

  //buffer to store points
  point points[numdiv][numdiv];

  for (int iu = 0; iu < numdiv; iu++) {
    u = iu * step;
    for (int iv = 0; iv < numdiv; iv++) {
      v = iv * step;
      //evaluate surface
      bezPatchInterp(&(points[iu][iv]), patch, u, v);
    }
  }

  //now generate triangle array from our buffer of points
  //2 triangles per quad
  triangle* my_triangles = (triangle*) malloc(2*(numdiv-1)*(numdiv-1) * sizeof(triangle));
  triangle_count = 2*(numdiv-1)*(numdiv-1);
  triangle* temp;
  for (int x = 0; x < numdiv - 1; x++) {
    for (int y = 0; y < numdiv - 1; y++) {
      
      //first triangle
      temp = &(my_triangles[ 2*(y+x*(numdiv-1)) ]);
      temp->a = points[x][y];
      temp->b = points[x][y+1];
      temp->c = points[x+1][y];
      temp->color = color;

      //second triangle
      temp = &(my_triangles[ 2*(y+x*(numdiv-1)) + 1 ]);
      temp->a = points[x+1][y];
      temp->b = points[x][y+1];
      temp->c = points[x+1][y+1];
      temp->color = color;

    }
  }
  return my_triangles;
}

triangle* subdivideAdaptive(patch* patch) {
  subdivideAdaptiveTriangle(patch, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, max_subdivisions);
  subdivideAdaptiveTriangle(patch, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, max_subdivisions);
  triangle_count = adaptiveTriangleList.size();
  triangle* my_triangles = (triangle*) malloc(triangle_count* sizeof(triangle));
  for(int i = 0; i < triangle_count; i++) {
    my_triangles[i] = adaptiveTriangleList.at(i);
  }
  return my_triangles;
}

void subdivideAdaptiveTriangle(patch* patch, float u1x, float u1y, float u2x, float u2y, float u3x, float u3y, int depth) {
  point p;
  vec3 x1; vec3 x1norm;
  vec3 x2; vec3 x2norm;
  vec3 x3; vec3 x3norm;
  bool e1;
  bool e2;
  bool e3;
  
  // get x1 x2 and x3
  bezPatchInterp(&p, patch, u1x, u1y);
  set(&x1, &(p.pos));
  set(&x1norm, &(p.norm));
  bezPatchInterp(&p, patch, u2x, u2y);
  set(&x2, &(p.pos));
  set(&x2norm, &(p.norm));
  bezPatchInterp(&p, patch, u3x, u3y);
  set(&x3, &(p.pos));
  set(&x3norm, &(p.norm));
  
  if(depth == 0) { // base case, create a triangle and add it to the arraylist
    triangle temp;
    set(&(temp.a.pos), &x1);
    set(&(temp.a.norm), &x1norm);
    
    set(&(temp.b.pos), &x2);
    set(&(temp.b.norm), &x2norm);
    
    set(&(temp.c.pos), &x3);
    set(&(temp.c.norm), &x3norm);
    
    //temp.color = color;
    adaptiveTriangleList.push_back(temp);
    return;
  }
  
  float xave;
  float yave;
  // x1 x2 test
  xave = (u1x + u2x) / 2;
  yave = (u1y + u2y) / 2;
  e2 = adaptiveTest(patch, xave, yave, &x1, &x2);
  
  // x1 x3 test
  xave = (u1x + u3x) / 2;
  yave = (u1y + u3y) / 2;
  e1 = adaptiveTest(patch, xave, yave, &x1, &x3);
  
  // x2 x3 test
  xave = (u1x + u3x) / 2;
  yave = (u1y + u3y) / 2;
  e3 = adaptiveTest(patch, xave, yave, &x2, &x3);
  
  if(e1 && e2 && e3) {
    // output as is
    triangle temp;
    set(&(temp.a.pos), &x1);
    set(&(temp.a.norm), &x1norm);
    
    set(&(temp.b.pos), &x2);
    set(&(temp.b.norm), &x2norm);
    
    set(&(temp.c.pos), &x3);
    set(&(temp.c.norm), &x3norm);
    
    //temp.color = color;
    adaptiveTriangleList.push_back(temp);
  } else if(e2 && e3) {
    float x = (u1x + u3x) / 2;
    float y = (u1y + u3y) / 2;
    subdivideAdaptiveTriangle(patch, u1x, u1y, u2x, u2y, x, y, depth - 1);
    subdivideAdaptiveTriangle(patch, x, y, u2x, u2y, u3x, u3y, depth - 1);
  } else if(e1 && e3) {
    float x = (u1x + u2x) / 2;
    float y = (u1y + u2y) / 2;
    subdivideAdaptiveTriangle(patch, u1x, u1y, x, y, u3x, u3y, depth - 1);
    subdivideAdaptiveTriangle(patch, x, y, u2x, u2y, u3x, u3y, depth - 1);
  } else if(e1 && e2) {
    float x = (u2x + u3x) / 2;
    float y = (u2y + u3y) / 2;
    subdivideAdaptiveTriangle(patch, u1x, u1y, x, y, u3x, u3y, depth - 1);
    subdivideAdaptiveTriangle(patch, u1x, u1y, u2x, u2y, x, y, depth - 1);
  } else if(e3) {
    float x1 = (u1x + u3x) / 2;
    float y1 = (u1y + u3y) / 2;
    float x2 = (u1x + u2x) / 2;
    float y2 = (u1y + u2y) / 2;
    subdivideAdaptiveTriangle(patch, u1x, u1y, x2, y2, x1, y1, depth - 1);
    subdivideAdaptiveTriangle(patch, x1, y1, x2, y2, u3x, u3y, depth - 1);
    subdivideAdaptiveTriangle(patch, x2, y2, u2x, u2y, u3x, u3y, depth - 1);
  } else if(e1) {
    float x1 = (u1x + u2x) / 2;
    float y1 = (u1y + u2y) / 2;
    float x2 = (u2x + u3x) / 2;
    float y2 = (u2y + u3y) / 2;
    subdivideAdaptiveTriangle(patch, u1x, u1y, x2, y2, u3x, u3y, depth - 1);
    subdivideAdaptiveTriangle(patch, u1x, u1y, x1, y1, x2, y2, depth - 1);
    subdivideAdaptiveTriangle(patch, x1, y1, u2x, u2y, x2, y2, depth - 1);
  } else if(e2) {
    float x1 = (u1x + u3x) / 2;
    float y1 = (u1y + u3y) / 2;
    float x2 = (u2x + u3x) / 2;
    float y2 = (u2y + u3y) / 2;
    subdivideAdaptiveTriangle(patch, u1x, u1y, u2x, u2y, x1, y1, depth - 1);
    subdivideAdaptiveTriangle(patch, x1, y1, x2, y2, u3x, u3y, depth - 1);
    subdivideAdaptiveTriangle(patch, x1, y1, u2x, u2y, x2, y2, depth - 1);
  } else {
    float x1 = (u1x + u2x) / 2;
    float y1 = (u1y + u2y) / 2;
    float x2 = (u1x + u3x) / 2;
    float y2 = (u1y + u3y) / 2;
    float x3 = (u2x + u3x) / 2;
    float y3 = (u2y + u3y) / 2;
    subdivideAdaptiveTriangle(patch, u1x, u1y, x1, y1, x2, y2, depth - 1);
    subdivideAdaptiveTriangle(patch, x2, y2, x3, y3, u3x, u3y, depth - 1);
    subdivideAdaptiveTriangle(patch, x2, y2, x3, y3, x1, y1, depth - 1);
    subdivideAdaptiveTriangle(patch, x1, y1, u2x, u2y, x3, y3, depth - 1);
  }
}

bool adaptiveTest(patch* patch, float xave, float yave, vec3* v1, vec3* v2) {
  point p;
  vec3 temp1;
  vec3 temp2;
  bezPatchInterp(&p, patch, xave, yave);
  set(&temp1, &(p.pos));
  add(&temp2, v1, v2);
  scale(&temp2, &temp2, 0.5f);
  subtract(&temp1, &temp1, &temp2);
  float mag = magnitude(&temp1);
  return mag < lambda;
}



//****************************************************
// Draw a random ass triangle 
//****************************************************

void drawTriangle(triangle *triangle) {
  glBegin(GL_TRIANGLES);

  //glColor3f(triangle->color->r, triangle->color->g, triangle->color->b);

  glNormal3f( triangle->a.norm.x, triangle->a.norm.y, triangle->a.norm.z );
  glVertex3f( triangle->a.pos.x, triangle->a.pos.y, triangle->a.pos.z );
  glNormal3f( triangle->b.norm.x, triangle->b.norm.y, triangle->b.norm.z );
  glVertex3f( triangle->b.pos.x, triangle->b.pos.y, triangle->b.pos.z );
  glNormal3f( triangle->c.norm.x, triangle->c.norm.y, triangle->c.norm.z );
  glVertex3f( triangle->c.pos.x, triangle->c.pos.y, triangle->c.pos.z );
  

  glEnd();
}

//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);             // indicate we are specifying camera transformations
  glLoadIdentity();               // make sure transformation is "zero'd"

  float xpos = vdistance * cos(angle);
  float ypos = 0.0f;
  float zpos = vdistance * sin(angle);

  gluLookAt(xpos, ypos, zpos,     // eye position
            0.0f, 0.0f, 0.0f,     // where to look at
            0.0f, 1.0f, 0.0f);    // up vector

  for (int x = 0; x < patch_count; x++) {
    for (int i = 0; i < triangle_counts[x]; i++) {
      drawTriangle(&(triangles[x][i]));
    }
  }

  glFlush();
  glutSwapBuffers();          // swap buffers (we earlier set double buffer)
}

patch* readPatches(char* filename) {
  std::ifstream bezierfile;
  bezierfile.open(filename);

  patch* patches;

  if (bezierfile.is_open()) {
    string line;
    char* word;
    char* cline;
    int count = 0;
    int subcount = 0;
    int currentPatch = 0;
    int curveCount = 0;
    int patchcount;
    while( getline(bezierfile, line) ) {
      if (count == 0) {
        patchcount = stoi(line);
        patch_count = patchcount;
        patches = (patch*)malloc(sizeof(patch)*patchcount);
      }
      else {
        if (count % 5 == 0 ) {
          //skip this line
        }
        else {
          currentPatch = (count) / 5;
          curveCount = (count - 1) % 5;
          if (currentPatch >= patchcount) {
            break;
          }

          cline = (char*)line.c_str();
          word = strtok (cline, " ");
          subcount = 0;
          while(word != NULL && subcount < 12) {
            if (subcount % 3 == 0) {
              //cout << currentPatch << "/" << curveCount << "/" << subcount / 3 << "/x " << stof(word) << "\n";
              patches[currentPatch].curves[curveCount].points[subcount / 3].x = stof(word);
            }
            else if (subcount % 3 == 1) {
              //cout << currentPatch << "/" << curveCount << "/" << subcount / 3 << "/y " << stof(word) << "\n";
              patches[currentPatch].curves[curveCount].points[subcount / 3].y = stof(word);
            }
            else {
              //cout << currentPatch << "/" << curveCount << "/" << subcount / 3 << "/z " << stof(word) << "\n";
              patches[currentPatch].curves[curveCount].points[subcount / 3].z = stof(word);
            }
            //cout << subcount << "/" << word << "\n";
            subcount++;
            word = strtok(NULL, " ");
          }
        }
      }
      count++;
    }
    bezierfile.close();
  }
  else {
    cout << "Unable to open file";
  }
  
  return patches;
}

void testBezCurveInterp() {
  curve temp;
  temp.points[0].x = 0.0f;
  temp.points[0].y = 0.0f;
  temp.points[0].z = 0.0f;
  
  temp.points[1].x = 1.0f;
  temp.points[1].y = 3.0f;
  temp.points[1].z = 0.0f;
  
  temp.points[2].x = 2.0f;
  temp.points[2].y = 3.0f;
  temp.points[2].z = 0.0f;
  
  temp.points[3].x = 3.0f;
  temp.points[3].y = 0.0f;
  temp.points[3].z = 0.0f;
  
  vec3 p;
  vec3 dPdu;
  for(float i = 0.0f; i <= 1.0f; i += 0.2f) {
    bezCurveInterp(&p, &dPdu, &temp, i);
    cout << "Point: (";
    cout << p.x << ", ";
    cout << p.y << ", ";
    cout << p.z << ")\n";
    cout << "Derivative: (";
    cout << dPdu.x << ", ";
    cout << dPdu.y << ", ";
    cout << dPdu.z << ")\n";
  }
}

void testBezPatchInterp() {
  patch patch;
  patch.curves[0].points[0].x = 0.0f;
  patch.curves[0].points[0].y = 0.0f;
  patch.curves[0].points[0].z = 0.0f;
  
  patch.curves[0].points[1].x = 0.0f;
  patch.curves[0].points[1].y = 1.0f;
  patch.curves[0].points[1].z = 0.0f;
  
  patch.curves[0].points[2].x = 0.0f;
  patch.curves[0].points[2].y = 2.0f;
  patch.curves[0].points[2].z = 0.0f;

  patch.curves[0].points[3].x = 0.0f;
  patch.curves[0].points[3].y = 3.0f;
  patch.curves[0].points[3].z = 0.0f;

  
  patch.curves[1].points[0].x = 1.0f;
  patch.curves[1].points[0].y = 0.0f;
  patch.curves[1].points[0].z = 0.0f;
  
  patch.curves[1].points[1].x = 1.0f;
  patch.curves[1].points[1].y = 1.0f;
  patch.curves[1].points[1].z = 3.0f;
  
  patch.curves[1].points[2].x = 1.0f;
  patch.curves[1].points[2].y = 2.0f;
  patch.curves[1].points[2].z = 3.0f;

  patch.curves[1].points[3].x = 1.0f;
  patch.curves[1].points[3].y = 3.0f;
  patch.curves[1].points[3].z = 0.0f;
 

  patch.curves[2].points[0].x = 2.0f;
  patch.curves[2].points[0].y = 0.0f;
  patch.curves[2].points[0].z = 0.0f;
  
  patch.curves[2].points[1].x = 2.0f;
  patch.curves[2].points[1].y = 1.0f;
  patch.curves[2].points[1].z = 4.0f;
  
  patch.curves[2].points[2].x = 2.0f;
  patch.curves[2].points[2].y = 2.0f;
  patch.curves[2].points[2].z = 5.0f;

  patch.curves[2].points[3].x = 2.0f;
  patch.curves[2].points[3].y = 3.0f;
  patch.curves[2].points[3].z = 0.0f;


  patch.curves[3].points[0].x = 3.0f;
  patch.curves[3].points[0].y = 0.0f;
  patch.curves[3].points[0].z = 0.0f;
  
  patch.curves[3].points[1].x = 3.0f;
  patch.curves[3].points[1].y = 1.0f;
  patch.curves[3].points[1].z = 0.0f;
  
  patch.curves[3].points[2].x = 3.0f;
  patch.curves[3].points[2].y = 2.0f;
  patch.curves[3].points[2].z = 0.0f;

  patch.curves[3].points[3].x = 3.0f;
  patch.curves[3].points[3].y = 3.0f;
  patch.curves[3].points[3].z = 0.0f;

  point p;
  for(float i = 0.0f; i <= 1.0f; i += 0.2f) {
    for(float j = 0.0f; j <= 1.0f; j += 0.2f) {
      bezPatchInterp(&p, &patch, i, j);
      //cout << "Point: (";
      cout << "[";
      cout << p.pos.x << ", ";
      cout << p.pos.y << ", ";
      cout << p.pos.z << "],";
      /*cout << "Normal: (";
      cout << p.norm.x << ", ";
      cout << p.norm.y << ", ";
      cout << p.norm.z << ")\n";*/
    }
  }
}


//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {

  //test bezCurveInterp
  //testBezCurveInterp();
  
  //test bezPatchInterp
  //testBezPatchInterp();
  
  

  //parse in command line arguments
  if (argc < 3) {
    cout << "prgm must take in at least 2 arguments\n";
    cout << "1st argument: .bez file\n";
    cout << "2nd argument: param constant\n";
    cout << "(optional) 3rd argument: -a (for adaptive)";
    exit(0);
  }
  char *bezfile = argv[1];
  float param = stof(argv[2]);

  bool adaptive = false;

  //ignore first arg - it is the name of this program
  int argI = 3;
  char *argument;
  while(argI < argc) {
    argument = argv[argI++];
    if (strcmp(argument, "-a") == 0) {
      //turn on 
      adaptive = true;
    }
    else {
      printf("Unknown argument %s\n", argv[argI++]);
    }
  }

  if (adaptive) {
    lambda = param;
  }
  else {
    step = param;
  }

  cout << "reading\n";

  //read in patch file
  patch* patches = readPatches(bezfile);

  //set up and generate triangles
  blue = (color*)malloc(sizeof(color));
  blue->r = 0.0f;
  blue->g = 0.0f;
  blue->b = 1.0f;

  red = (color*)malloc(sizeof(color));
  red->r = 1.0f;
  red->g = 0.0f;
  red->b = 0.0f;

  
  cout << "This needs work - go through all patches + amalgate traingels\n";

  triangles = (triangle**) malloc(sizeof(triangle*) * patch_count);
  triangle_counts = (int*) malloc(sizeof(int)*patch_count);

  if (adaptive) {
    for (int i = 0; i < patch_count; i++) {
      if (i % 2 == 0) {
        triangles[i] = subdivideAdaptive(&(patches[i]));
        triangle_counts[i] = triangle_count;
      }
      else {
        triangles[i] = subdivideAdaptive(&(patches[i]));
        triangle_counts[i] = triangle_count;
      }
    }
  }
  else {
    for (int i = 0; i < patch_count; i++) {
      if (i % 2 == 0) {
        triangles[i] = subdivideUniform(&(patches[i]), step, blue);
        triangle_counts[i] = triangle_count;
      }
      else {
        triangles[i] = subdivideUniform(&(patches[i]), step, red);
        triangle_counts[i] = triangle_count;
      }
    }
  }

  

  //triangle_count = sizeof(*triangles) / sizeof(triangle);


  //This initializes glut
  glutInit(&argc, argv);

  //This tells glut to use a double-buffered window with red, green, and blue channels 
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  // Initalize theviewport size
  viewport.w = 700;
  viewport.h = 700;

  //The size and position of the window
  glutInitWindowSize(viewport.w, viewport.h);
  glutInitWindowPosition(0,0);
  glutCreateWindow(argv[0]);

  initScene();              // quick function to set up scene

  glutDisplayFunc(myDisplay);       // function to run when its time to draw something
  glutReshapeFunc(myReshape);       // function to run when the window gets resized
  glutKeyboardFunc(myKeyPressed);
  glutSpecialFunc(mySpecialInput);

  glutMainLoop();             // infinite loop that will keep drawing and resizing
  // and whatever else

  return 0;
}