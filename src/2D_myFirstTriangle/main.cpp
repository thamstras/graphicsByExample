// tag::C++11check[]
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if __cplusplus < 201103L
  #pragma message("WARNING: the compiler may not be C++11 compliant. __cplusplus version is : " STRING(__cplusplus))
#endif
// end::C++11check[]

// tag::includes[]
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <time.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <SDL.h>
// end::includes[]

// tag::namespace[]
using namespace std;
// end::namespace[]


// tag::globalVariables[]
std::string exeName;
SDL_Window *win; //pointer to the SDL_Window
SDL_GLContext context; //the SDL_GLContext
int frameCount = 0;
std::string frameLine = "";
// end::globalVariables[]

// tag::vertexShader[]
//string holding the **source** of our vertex shader, to save loading from a file
const std::string strVertexShader = R"(
	#version 330
	in vec2 position;
	in vec3 in_Color;
	out vec3 pass_Color;
	uniform vec2 offset;
	void main()
	{
		pass_Color = in_Color;
		vec2 tmpPosition = position + offset;
		gl_Position = vec4(tmpPosition, 0.0, 1.0);
	}
)";
// end::vertexShader[]

// tag::fragmentShader[]
//string holding the **source** of our fragment shader, to save loading from a file
const std::string strFragmentShader = R"(
	#version 330
	out vec4 outputColor;
	in vec3 pass_Color;
	void main()
	{
	   outputColor = vec4(pass_Color, 1.0);
	}
)";
// end::fragmentShader[]

//our variables
bool done = false;
clock_t lastFrame_t = 0, thisFrame_t = 0;
double sim_t = 0;

//the data about our geometry
const GLfloat vertexData[] = {
	 0.000f,  0.500f,
	-0.433f, -0.250f,
	 0.433f, -0.250f,
};

//the data about our color
const GLfloat colorData[] = {
	1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f
};

//the offset we'll pass to the GLSL
GLfloat offset[] = { -0.5, -0.5 }; //using different values from CPU and static GLSL examples, to make it clear this is working
GLfloat offsetVelocity[] = { 0.2, 0.2 }; //rate of change of offset in units per second

GLfloat color[] = { 1.0, 0.0, 0.0, 1.0 }; //set the initial color. (1, 0, 0, 1) ie: Red.

//our GL and GLSL variables

GLuint theProgram; //GLuint that we'll fill in to refer to the GLSL program (only have 1 at this point)
GLint positionLocation; //GLuint that we'll fill in with the location of the `offset` variable in the GLSL
GLint offsetLocation; //GLuint that we'll fill in with the location of the `offset` variable in the GLSL
GLint colorLocation;

GLuint vertexDataBufferObject;
GLuint vertexColorBufferObject;
GLuint vertexArrayObject;

// end Global Variables
/////////////////////////


void initialise()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		exit(1);
	}
	cout << "SDL initialised OK!\n";
}

void createWindow()
{
	//get executable name, and use as window title
	int beginIdxWindows = exeName.rfind("\\"); //find last occurrence of a backslash
	int beginIdxLinux = exeName.rfind("/"); //find last occurrence of a forward slash
	int beginIdx = max(beginIdxWindows, beginIdxLinux);
	std::string exeNameEnd = exeName.substr(beginIdx + 1);
	const char *exeNameCStr = exeNameEnd.c_str();

	//create window
	win = SDL_CreateWindow(exeNameCStr, 100, 100, 600, 600, SDL_WINDOW_OPENGL); //same height and width makes the window square ...

	//error handling
	if (win == nullptr)
	{
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "SDL CreatedWindow OK!\n";
}

void setGLAttributes()
{
  int major = 3;
	int minor = 3;
	cout << "Built for OpenGL Version " << major << "." << minor << endl; //ahttps://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
	// set the opengl context version
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //core profile
	cout << "Set OpenGL context to versicreate remote branchon " << major << "." << minor << " OK!\n";
}

void createContext()
{
	setGLAttributes();

	context = SDL_GL_CreateContext(win);
	if (context == nullptr){
		SDL_DestroyWindow(win);
		std::cout << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "Created OpenGL context OK!\n";
}

void initGlew()
{
	GLenum rev;
	glewExperimental = GL_TRUE; //GLEW isn't perfect - see https://www.opengl.org/wiki/OpenGL_Loading_Library#GLEW
	rev = glewInit();
	if (GLEW_OK != rev){
		std::cout << "GLEW Error: " << glewGetErrorString(rev) << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLEW Init OK!\n";
	}
}

GLuint createShader(GLenum eShaderType, const std::string &strShaderFile)
{
	GLuint shader = glCreateShader(eShaderType);
	//error check
	const char *strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch (eShaderType)
		{
		case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
		case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return shader;
}

GLuint createProgram(const std::vector<GLuint> &shaderList)
{
	GLuint program = glCreateProgram();

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glAttachShader(program, shaderList[iLoop]);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glDetachShader(program, shaderList[iLoop]);

	return program;
}

void initializeProgram()
{
	std::vector<GLuint> shaderList;

	shaderList.push_back(createShader(GL_VERTEX_SHADER, strVertexShader));
	shaderList.push_back(createShader(GL_FRAGMENT_SHADER, strFragmentShader));

	theProgram = createProgram(shaderList);
	if (theProgram == 0)
	{
		cout << "GLSL program creation error." << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLSL program creation OK! GLUint is: " << theProgram << std::endl;
	}

	positionLocation = glGetAttribLocation(theProgram, "position");
	offsetLocation = glGetUniformLocation(theProgram, "offset");
	colorLocation = glGetAttribLocation(theProgram, "in_Color");
	//clean up shaders (we don't need them anymore as they are no in theProgram
	for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
}

void initializeVertexBuffer()
{
	glGenBuffers(1, &vertexDataBufferObject);

	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexDataBufferObject created OK! GLUint is: " << vertexDataBufferObject << std::endl;

	glGenBuffers(1, &vertexColorBufferObject);

	glBindBuffer(GL_ARRAY_BUFFER, vertexColorBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colorData), colorData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexColorBufferObject created OK! GLuint is: " << vertexColorBufferObject << std::endl;
}

void loadAssets()
{
	initializeProgram(); //create GLSL Shaders, link into a GLSL program, and get IDs of attributes and variables

	initializeVertexBuffer(); //load data into a vertex buffer

	cout << "Loaded Assets OK!\n";
}

void setupvertexArrayObject()
{
	glGenVertexArrays(1, &vertexArrayObject); //create a Vertex Array Object
	cout << "Vertex Array Object created OK! GLUint is: " << vertexArrayObject << std::endl;

	glBindVertexArray(vertexArrayObject); //make the just created vertexArrayObject the active one

		glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject); //bind vertexDataBufferObject

		glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation

		glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation


		glBindBuffer(GL_ARRAY_BUFFER, vertexColorBufferObject);

		glEnableVertexAttribArray(colorLocation);

		glVertexAttribPointer(colorLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it

	//cleanup
	glDisableVertexAttribArray(positionLocation); //disable vertex attribute at index positionLocation
	glDisableVertexAttribArray(colorLocation);
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

}

void changeColor()
{
	for (int i = 0; i < 3; i++)
	{
		color[i] = ((float)rand()) / (float)RAND_MAX;
	}
}

void handleInput()
{
	//Event-based input handling
	//The underlying OS is event-based, so **each** key-up or key-down (for example)
	//generates an event.
	//  - https://wiki.libsdl.org/SDL_PollEvent
	//In some scenarios we want to catch **ALL** the events, not just to present state
	//  - for instance, if taking keyboard input the user might key-down two keys during a frame
	//    - we want to catch based, and know the order
	//  - or the user might key-down and key-up the same within a frame, and we still want something to happen (e.g. jump)
	//  - the alternative is to Poll the current state with SDL_GetKeyboardState

	SDL_Event event; //somewhere to store an event

	//NOTE: there may be multiple events per frame
	while (SDL_PollEvent(&event)) //loop until SDL_PollEvent returns 0 (meaning no more events)
	{
		switch (event.type)
		{
		case SDL_QUIT:
			done = true; //set donecreate remote branch flag if SDL wants to quit (i.e. if the OS has triggered a close event,
							//  - such as window close, or SIGINT
			break;

			//keydown handling - we should to the opposite on key-up for direction controls (generally)
		case SDL_KEYDOWN:
			//Keydown can fire repeatable if key-repeat is on.
			//  - the repeat flag is set on the keyboard event, if this is a repeat event
			//  - in our case, we're going to ignore repeat events
			//  - https://wiki.libsdl.org/SDL_KeyboardEvent
			if (!event.key.repeat)
				switch (event.key.keysym.sym)
				{
					//hit escape to exit
					case SDLK_ESCAPE:
						done = true;
						break;
					case SDLK_RETURN:
						//changeColor(); //disabled for time based color change.
						break;
				}
			break;
		}
	}
}

void updateSimulation(double simLength) //update simulation with an amount of time to simulate for (in seconds)
{
	sim_t += simLength;
	offset[0] = sin(sim_t) / 2; // /2 so triangle stays on screen.
	color[0] = sin(sim_t) + 0.5f;
}

void preRender()
{
	glViewport(0, 0, 600, 600); //set viewpoint
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f); //set clear colour
	glClear(GL_COLOR_BUFFER_BIT); //clear the window (technical the scissor box bounds)
}

void render()
{
	glUseProgram(theProgram); //installs the program object specified by program as part of current rendering state

	//load data to GLSL that **may** have changed
	glUniform2f(offsetLocation, offset[0], offset[1]);
		//alternatively, use glUnivform2fv
		//glUniform2fv(offsetLocation, 1, offset); //Note: the count is 1, because we are setting a single uniform vec2 - https://www.opengl.org/wiki/GLSL_:_common_mistakes#How_to_use_glUniform

	//glUniform4fv(colorLocation, 1, color);

	glBindVertexArray(vertexArrayObject);

	glDrawArrays(GL_TRIANGLES, 0, 3); //Draw something, using Triangles, and 3 vertices - i.e. one lonely triangle

	glBindVertexArray(0);

	glUseProgram(0); //clean up

}

void postRender()
{
	SDL_GL_SwapWindow(win);; //present the frame buffer to the display (swapBuffers)
	
	thisFrame_t = clock();

	double frameTime = (thisFrame_t - lastFrame_t) / (double)CLOCKS_PER_SEC;
	lastFrame_t = thisFrame_t;
	double fps = 1 / frameTime;
	
	frameLine += "Frame: " + std::to_string(frameCount++);
	frameLine += "\tFrameTime: " + std::to_string(frameTime);
	frameLine += "\tFPS: " + std::to_string(fps);
	cout << "\r" << frameLine << std::flush;
	frameLine = "";
}

void cleanUp()
{
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	cout << "Cleaning up OK!\n";
}

int main( int argc, char* args[] )
{
	exeName = args[0];
	//setup
	//- do just once
	initialise();
	createWindow();

	createContext();

	initGlew();

	glViewport(0,0,600,600); //should check what the actual window res is?

  SDL_GL_SwapWindow(win); //force a swap, to make the trace clearer

	//do stuff that only needs to happen once
	//- create shaders
	//- load vertex data
	loadAssets();

	//setup a GL object (a VertexArrayObject) that stores how to access data and from where
	setupvertexArrayObject();

	while (!done) //loop until done flag is set)
	{
		handleInput();

		updateSimulation(0.02); //call update simulation with an amount of time to simulate for (in seconds)
		  //WARNING - we are always updating by a constant amount of time. This should be tied to how long has elapsed
		    // see, for example, http://headerphile.blogspot.co.uk/2014/07/part-9-no-more-delays.html

		preRender();

		render(); //RENDER HERE - PLACEHOLDER

		postRender();

	}

	//cleanup and exit
	cleanUp();
	SDL_Quit();

	return 0;
}
