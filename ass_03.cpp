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
#include <float.h>

#define PI 3.14159265  // Should be used from mathlib
#define COLOR_MAX 255

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

//****************************************************
// Global Variables
//****************************************************
Viewport  viewport;

//****************************************************
// Simple init function
//****************************************************
void initScene(){

  // Nothing to do here for this simple example.

}


//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
  viewport.w = w;
  viewport.h = h;

  glViewport (0,0,viewport.w,viewport.h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //gluOrtho2D(0, viewport.w, 0, viewport.h);
  gluOrtho2D(0, 1.0f, 0, 1.0f);
}

void myKeyPressed(unsigned char key, int x, int y) {
  if(key == 32) {
    exit(0);
  }
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

//****************************************************
// Draw a random ass rectangle 
//****************************************************


void drawRectangle(float x, float y, float width, float height, float r, float g, float b) {
  glColor3f(r,g,b);                   // setting the color to w/e for the rect
  glBegin(GL_POLYGON);                         // draw rectangle 
  //glVertex3f(x val, y val, z val (won't change the point because of the projection type));
  glVertex3f(x, y , 0.0f);               // bottom left corner of rectangle
  glVertex3f(x, y + height, 0.0f);               // top left corner of rectangle
  glVertex3f(x + width, y + height, 0.0f);               // top right corner of rectangle
  glVertex3f(x + width, y, 0.0f);               // bottom right corner of rectangle
  glEnd();
}

//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // clear the color buffer

  glMatrixMode(GL_MODELVIEW);             // indicate we are specifying camera transformations
  glLoadIdentity();               // make sure transformation is "zero'd"


  //test draw

  glBegin(GL_QUADS);
               glColor3f(0.5f, 0.0f, 1.0f); // make this vertex purple
               glVertex2f(-0.75, 0.75);
               glColor3f(1.0f, 0.0f, 0.0f); // make this vertex red
               glVertex2f(-0.75, -0.75);
               glColor3f(0.0f, 1.0f, 0.0f); // make this vertex green
               glVertex2f(0.75, -0.75);
               glColor3f(1.0f, 1.0f, 0.0f); // make this vertex yellow
               glVertex2f(0.75, 0.75);
  glEnd();
  // Start drawing
  //circle(viewport.w / 2.0 , viewport.h / 2.0 , min(viewport.w, viewport.h) * 0.45f );

  glFlush();
  glutSwapBuffers();          // swap buffers (we earlier set double buffer)
}



//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {
  //ignore first arg - it is the name of this program
  int argI = 1;
  
  char *argument;
  //float divideBy = FLT_MAX;
  float divideBy = 1.0f;
  while(argI < argc) {
    argument = argv[argI++];
    if (strcmp(argument, "-ka") == 0) {
      //stuff
    }
    else if (strcmp(argument, "-kd") == 0) {
      //stuff
    }
    else {
      printf("Unknown argument %s\n", argv[argI++]);
    }
  }

  //This initializes glut
  glutInit(&argc, argv);

  //This tells glut to use a double-buffered window with red, green, and blue channels 
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  // Initalize theviewport size
  viewport.w = 400;
  viewport.h = 400;

  //The size and position of the window
  glutInitWindowSize(viewport.w, viewport.h);
  glutInitWindowPosition(0,0);
  glutCreateWindow(argv[0]);

  initScene();              // quick function to set up scene

  glutDisplayFunc(myDisplay);       // function to run when its time to draw something
  glutReshapeFunc(myReshape);       // function to run when the window gets resized
  glutKeyboardFunc(myKeyPressed);

  glutMainLoop();             // infinite loop that will keep drawing and resizing
  // and whatever else

  return 0;
}