/**
 * This file accompanies the tutorial "Strange Attractors in C++ and OpenGL"
 * available at http://nathanselikoff.com/tutorial-strange-attractors-in-c-and-opengl
 * 
 * This application renders a Pickover strange attractor, "The King's Dream", the
 * parameters of which are found on page 27 of "Chaos in Wonderland" by Clifford Pickover
 * 
 * This file is released under Creative Commons Attribution Noncommercial Share-Alike 3.0 License
 * http://creativecommons.org/licenses/by-nc-sa/3.0/
 * You may modify this code, but you may not republish it without crediting me. Thank you.
 * 
 * Created by Nathan Selikoff on 4/13/11
 * http://nathanselikoff.com
 * 
 */
// Used this for generating the images
//https://stackoverflow.com/questions/3191978/how-to-use-glut-opengl-to-render-to-a-file

// What I have here is some combination of the stuff from the two sources above, the code needs some cleaning, 
// I will probably give this some more work in the future...
// - gumeo

#include <iostream>
#include <iomanip>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

enum Constants { SCREENSHOT_MAX_FILENAME = 256 };
static GLubyte *pixels = NULL;
static GLuint fbo;
static GLuint rbo_color;
static GLuint rbo_depth;
static const unsigned int HEIGHT = 1000;
static const unsigned int WIDTH = 1000;
static int offscreen = 1;
static unsigned int max_nframes = 600;
static unsigned int nframes = 0;
static unsigned int time0;


float	x = 0.1, y = 0.1,			// starting point
		a = -0.966918,				// coefficients for "The King's Dream"
		b = 2.879879,
		c = 0.58,//c = 0.765145,
		d = 0.813;//d = 0.744728;
int		initialIterations = 100,	// initial number of iterations to allow the attractor to settle
		iterations = 2000000;		// number of times to iterate through the functions and draw a point

static double angle;
static double delta_angle;
/*
Take screenshot with glReadPixels and save to a file in PPM format.
-   filename: file path to save to, without extension
-   width: screen width in pixels
-   height: screen height in pixels
-   pixels: intermediate buffer to avoid repeated mallocs across multiple calls.
    Contents of this buffer do not matter. May be NULL, in which case it is initialized.
    You must `free` it when you won't be calling this function anymore.
*/
static void screenshot_ppm(const char *filename, unsigned int width,
        unsigned int height, GLubyte **pixels) {
    size_t i, j, k, cur;
    const size_t format_nchannels = 3;
    FILE *f = fopen(filename, "w");
    fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);
    *pixels = (GLubyte*)realloc(*pixels, format_nchannels * sizeof(GLubyte) * width * height);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, *pixels);
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            cur = format_nchannels * ((height - i - 1) * width + j);
            fprintf(f, "%3d %3d %3d ", (*pixels)[cur], (*pixels)[cur + 1], (*pixels)[cur + 2]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

static int model_init(void) {
    x = 0.1, y = 0.1,                       // starting point
                a = -0.966918,                          // coefficients for "The King's Dream"
                b = 2.879879,
                c = 0.58,//c = 0.765145,
                d = 0.813;//d = 0.744728;
  delta_angle = 0.001;
}

static int model_update(void) {
    // This has to be made more general for defining any loop in the parameters space
    x = 0.1, y = 0.1;
    if(nframes < 100){
      a += delta_angle;
      b += delta_angle;
      return 0;
    }
    if(nframes < 200){
      b += delta_angle;
      c += delta_angle/8;
      return 0;
    }
    if(nframes < 300){
      c += delta_angle/8;
      d += delta_angle;
      return 0;
    }
    if(nframes < 400){
      a -= delta_angle;
      b -= delta_angle;
      return 0;
    }
    if(nframes < 500){
      b -= delta_angle;
      c -= delta_angle/8;
      return 0;
    }
    if(nframes < 600){
      c -= delta_angle/8;
      d -= delta_angle;
      return 0;
    }
    return 0;
}

static int model_finished(void) {
    return nframes >= max_nframes;
}

void myinit() {

	// set the background color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	// set the foreground (pen) color
	glColor4f(1.0f, 1.0f, 1.0f, 0.02f);
	
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
	// set up the viewport
	glViewport(0, 0, WIDTH, HEIGHT);
	
	// set up the projection matrix (the camera)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-2.0f, 2.0f, -2.0f, 2.0f);
	
	// set up the modelview matrix (the objects)
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
        time0 = glutGet(GLUT_ELAPSED_TIME);
        model_init();


	// compute some initial iterations to settle into the orbit of the attractor
	for (int i = 0; i < initialIterations; i++) {
		
		// compute a new point using the strange attractor equations
		float xnew = sin(y*b) + c*sin(x*b);
		float ynew = sin(x*a) + d*sin(y*a);
		
		// save the new point
		x = xnew;
		y = ynew;
	}
	
	// enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// enable point smoothing
	glEnable(GL_POINT_SMOOTH);	
	glPointSize(1.0f); // Doubled the point size
}

static void deinit(void){
  printf("FPS = %f\n", 1000.0 * nframes / (double)(glutGet(GLUT_ELAPSED_TIME) - time0));
  free(pixels);
}

static void draw_scene(){
    glClear(GL_COLOR_BUFFER_BIT);

    // draw some points
    glBegin(GL_POINTS);

    // iterate through the equations many times, drawing one point for each iteration
    for (int i = 0; i < iterations; i++) {

        // compute a new point using the strange attractor equations
        float xnew = sin(y*b) + c*sin(x*b);
        float ynew = sin(x*a) + d*sin(y*a);

        // save the new point
        x = xnew;
        y = ynew;

        // draw the new point
        glVertex2f(x, y);
        if(i % 1000000 == 0){
            std::cout << "Iterations number: " << i << std::endl;
        }
    }
    glEnd();
}

void mydisplay() {

    char extension[SCREENSHOT_MAX_FILENAME];
    char filename[SCREENSHOT_MAX_FILENAME];
    draw_scene();
    glutSwapBuffers();
    std::ostringstream ss;
    ss << std::setw(5) << std::setfill('0') << nframes;;
    std::string s2(ss.str());
    s2 += ".ppm";
    const char* val = s2.c_str(); 	
    snprintf(filename, SCREENSHOT_MAX_FILENAME,"%s", val);
    //screenshot_ppm(filename, WIDTH, HEIGHT, &pixels);
	
    nframes++;
    if (model_finished())
        exit(EXIT_SUCCESS);
}

static void idle(void) {
    while (model_update());
    glutPostRedisplay();
}

void mykey(unsigned char mychar, int x, int y) {
	
	// exit the program when the Esc key is pressed
	if (mychar == 27) {
		exit(0);
	}
	
}

int main (int argc, char **argv) {
	GLint glut_display;
        glut_display = GLUT_DOUBLE;
	// initialize GLUT
	glutInit(&argc, argv);
	
	// set up our display mode for color with alpha and double buffering
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	
	// create a 400px x 400px window
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Funky looking strange attractors!");
	
	// register our callback functions
	glutDisplayFunc(mydisplay);
        glutIdleFunc(idle);
        atexit(deinit);
	glutKeyboardFunc(mykey);
        
	// call our initialization function
	myinit();
	
	// start the program
	glutMainLoop();
	

    return 0;
}
