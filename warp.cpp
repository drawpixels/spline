/*
	warp.cpp
	Warpping the grid with mouse input.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
GLFWwindow* window;

#include <glm/glm.hpp>
using namespace glm;

#include "Grid.hpp"
#include "ControlPoint.hpp"

char sWindowTitle[] = "WARP";
float win_width = 800.0f;
float win_height = 800.0f;
const char* vertex_shader =
"#version 400\n"
"in vec3 vp;"
"void main () {"
"  gl_Position = vec4 (vp, 1.0);"
"}";
const char* fragment_shader =
"#version 400\n"
"uniform vec3 color;"
"out vec4 frag_colour;"
"void main () {"
"  frag_colour = vec4 (color.r, color.g, color.b, 1.0);"
"}";

Grid* grid;
#define GRID_SZ 50
#define GRID_MIN -0.95f
#define GRID_MAX 0.95f

std::vector<ControlPoint> controlPoints;

void framebuffer_size_callback(GLFWwindow*, int, int);
void key_callback (GLFWwindow*, int, int, int, int);
void cursor_pos_callback (GLFWwindow*, double, double);
void button_callback (GLFWwindow*, int, int, int);
GLuint LoadSimpleShader (const char* vert, const char* frag);
GLuint CreateRect ();

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}
	// This is requied for OS X
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( (int)win_width, (int)win_height, sWindowTitle, NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
	glfwSetKeyCallback(window,key_callback);
	//glfwSetCursorPosCallback(window,cursor_pos_callback);
	glfwSetMouseButtonCallback(window,button_callback);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Create and compile our GLSL program from the shaders
	GLuint programme = LoadSimpleShader(vertex_shader,fragment_shader);
    glUseProgram (programme);
	GLuint ColorID = glGetUniformLocation(programme, "color");

	grid = new Grid(GRID_SZ, GRID_MIN, GRID_MAX);
	GLuint VAOrect = CreateRect();

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

	while( !glfwWindowShouldClose(window) ) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUniform3f(ColorID, 1.0f, 0.0f, 1.0f);

		glBindVertexArray(VAOrect);
		glDrawArrays(GL_TRIANGLES,0,3);

		glUniform3f(ColorID, 0.8f, 0.8f, 0.8f);
		grid->Display();
		
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} 

	for (std::vector<ControlPoint>::iterator it=controlPoints.begin(); it!=controlPoints.end(); ++it) {
		vec2 begin = it->Begin();
		vec2 e = it->End();
		printf("%6.3f,%6.3f - %6.3f,%6.3f\n",begin.x,begin.y,e.x,e.y);
	}

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

//---------------------------------
// Supporting call-back functions
//---------------------------------
void framebuffer_size_callback (GLFWwindow* window, int width, int height) {
	win_width = (double)width;
	win_height = (double)height;
//	glViewport(0,0,width,height);
}

void key_callback (GLFWwindow* window, int key, int scancode, int action, int mods) {
	//printf("KEY - %d %d %d %d\n",key,scancode,action,mods);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void cursor_pos_callback (GLFWwindow* window, double x, double y) {
	bool bMB1 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_1)==GLFW_PRESS);
	bool bMB2 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_2)==GLFW_PRESS);
	bool bMB3 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_3)==GLFW_PRESS);
	bool bCtrl = (glfwGetKey(window,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) || (glfwGetKey(window,GLFW_KEY_RIGHT_CONTROL)==GLFW_PRESS);
}

void ScalePosition (double mouse_x, double mouse_y, float& scaled_x, float& scaled_y) {
	scaled_x = float(mouse_x / win_width) * 2.0f - 1.0f;
	scaled_y = 1.0f - float(mouse_y / win_height) * 2.0f;
}

void button_callback (GLFWwindow* window, int button, int action, int mods) {
	double x,y;
	glfwGetCursorPos(window,&x,&y);
	//printf("BUTTON - %6.1f %6.1f %d %d %d\n",x,y,button,action,mods);
	if (button==GLFW_MOUSE_BUTTON_1 && action==GLFW_PRESS) {
		float sx,sy;
		ScalePosition(x,y,sx,sy);
		ControlPoint ctrlPt(sx,sy);
		controlPoints.push_back(ctrlPt);
		std::cout << "P:" << x << "," << y << " -- " << sx << "," << sy << std::endl;
	} 
	if (button==GLFW_MOUSE_BUTTON_1 && action==GLFW_RELEASE) {
		float sx,sy;
		ScalePosition(x,y,sx,sy);
		if ((sx<GRID_MIN) || (sx>GRID_MAX) || (sy<GRID_MIN) || (sy>GRID_MAX))
			controlPoints.pop_back();	// control point is invalid - DISCARD
		else
			controlPoints.back().SetEnd(sx,sy);
		std::cout << "R:" << x << "," << y << " -- " << sx << "," << sy << std::endl;
	} 
}

GLuint LoadSimpleShader (const char* vert, const char* frag) {
    GLuint vs = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (vs, 1, &vert, NULL);
    glCompileShader (vs);
    GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (fs, 1, &frag, NULL);
    glCompileShader (fs);
    GLuint shader_programme = glCreateProgram ();
    glAttachShader (shader_programme, fs);
    glAttachShader (shader_programme, vs);
    glLinkProgram (shader_programme);
	return shader_programme;
}


GLuint CreateGrid() {
	
	unsigned int indices[GRID_SZ*(GRID_SZ+1)*4];
	GLfloat vertices[(GRID_SZ+1)*(GRID_SZ+1)*3];
	GLfloat step = (GRID_MAX - GRID_MIN) / (GLfloat)GRID_SZ;
	
	// Define vertex positions on the sides of the frame
	int idx = 0;
	for (int i=0; i<=GRID_SZ; i++) {
		for (int j=0; j<=GRID_SZ; j++) {
			vertices[idx++] = GRID_MIN + step * (float)j;
			vertices[idx++] = GRID_MIN + step * (float)i;
			vertices[idx++] = 0.0f;
		}
	}
	// Define the connection sequence
	idx = 0;
	for (int i=0; i<=GRID_SZ; i++) {
		for (int j=0; j<=GRID_SZ; j++) {
			if (j!=GRID_SZ) {
				indices[idx++] = i * (GRID_SZ+1) + j;
				indices[idx++] = i * (GRID_SZ+1) + (j+1);
			}
			if (i!=GRID_SZ) {
				indices[idx++] = i * (GRID_SZ+1) + j;
				indices[idx++] = (i+1) * (GRID_SZ+1) + j;
			}
		}
	}

	GLuint Vao;
	glGenVertexArrays(1, &Vao);
	glBindVertexArray(Vao);

	GLuint Vbo[2];
	glGenBuffers(2, Vbo);
	glBindBuffer(GL_ARRAY_BUFFER, Vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Vbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);

	return Vao;
}

GLuint CreateRect() {
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	static const GLfloat g_vertex_buffer_data[] = { 
		0.433f, 0.152f, 0.0f,
		0.624f, 0.129f, 0.0f,
		0.491f, 0.321f, 0.0f,
	};
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	return VertexArrayID;
}


