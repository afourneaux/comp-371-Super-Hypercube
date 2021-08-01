// COMP 371 Assignment 2
// Spiral Staircase (Section DD Team 3)
// 
// Badele, Theodor (40129466)
// Fourneaux, Alexander (40022711)
// Reda, Antonio (40155615)
// Zhang, Chi (29783539)
// 
// Based on Labs Framework created by Nicolas Bergeron
// Lighting implemented via a guide on https://learnopengl.com
//
// CONTRIBUTIONS
//
//  Theodor
// 
//  Alexander
//
//  Antonio
//
//  Chi
//

#include <iostream>
#include <list>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#define GLEW_STATIC 1   // This allows linking with Static Library on Windows, without DLL
#include <GL/glew.h>    // Include GLEW - OpenGL Extension Wrangler

#include <GLFW/glfw3.h> // cross-platform interface for creating a graphical context,
						// initializing OpenGL and binding inputs

#include <glm/glm.hpp>  // GLM is an optimized math library with syntax to similar to OpenGL Shading Language
#include <glm/gtc/matrix_transform.hpp> // include this to create transformation matrices
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Constants.h"
#include "Shape.h"
#include "ShaderManager.h"
#include "Coordinates.h"
#include "ShaderLoader.h"	
#include "ControlState.h"
#include "Wall.h"

using namespace glm;
using namespace std;

/////////////////////// MAIN ///////////////////////

void windowSizeCallback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

int createVertexArrayObjectColoured(vec3 frontBackColour, vec3 topBottomColour, vec3 leftRightColour);
int createVertexArrayObjectSingleColoured(vec3 colour);

bool initContext();

// shader variable setters
void SetUniformMat4(GLuint shader_id, const char* uniform_name,
	mat4 uniform_value) {
	glUseProgram(shader_id);
	glUniformMatrix4fv(glGetUniformLocation(shader_id, uniform_name), 1, GL_FALSE,
		&uniform_value[0][0]);
}

void SetUniformVec3(GLuint shader_id, const char* uniform_name,
	vec3 uniform_value) {
	glUseProgram(shader_id);
	glUniform3fv(glGetUniformLocation(shader_id, uniform_name), 1,
		value_ptr(uniform_value));
}

template <class T>
void SetUniform1Value(GLuint shader_id, const char* uniform_name,
	T uniform_value) {
	glUseProgram(shader_id);
	glUniform1i(glGetUniformLocation(shader_id, uniform_name), uniform_value);
	glUseProgram(0);
}

GLFWwindow* window = NULL;

int main(int argc, char* argv[])
{
	if (!initContext()) return -1;

	// Black background
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// We can set the shader once, since we have only one
	ShaderManager shaderManager = ShaderManager(VERTEX_SHADER_FILEPATH, FRAGMENT_SHADER_FILEPATH);
	shaderManager.use();



	std::string shaderPathPrefix = "../Assets/Shaders/";

	GLuint shaderScene = loadSHADER(shaderPathPrefix + "scene_vertex.glsl",
		shaderPathPrefix + "scene_fragment.glsl");

	GLuint shaderShadow = loadSHADER(shaderPathPrefix + "shadow_vertex.glsl",
		shaderPathPrefix + "shadow_fragment.glsl");







	// Dimensions of the shadow texture, which should cover the viewport window
	// size and shouldn't be oversized and waste resources
	const unsigned int DEPTH_MAP_TEXTURE_SIZE = 1024;

	// Variable storing index to texture used for shadow mapping
	GLuint depth_map_texture;
	// Get the texture
	glGenTextures(1, &depth_map_texture);

	// Bind the texture so the next glTex calls affect it
	glBindTexture(GL_TEXTURE_2D, depth_map_texture);
	// Create the texture and specify it's attributes, including widthn height,
	// components (only depth is stored, no color information)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DEPTH_MAP_TEXTURE_SIZE,
		DEPTH_MAP_TEXTURE_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	// Set texture sampler parameters.
	// The two calls below tell the texture sampler inside the shader how to
	// upsample and downsample the texture. Here we choose the nearest filtering
	// option, which means we just use the value of the closest pixel to the
	// chosen image coordinate.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// The two calls below tell the texture sampler inside the shader how it
	// should deal with texture coordinates outside of the [0, 1] range. Here we
	// decide to just tile the image.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Variable storing index to framebuffer used for shadow mapping
	GLuint depth_map_fbo;  // fbo: framebuffer object
	// Get the framebuffer
	glGenFramebuffers(1, &depth_map_fbo);
	// Bind the framebuffer so the next glFramebuffer calls affect it
	glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
	// Attach the depth map texture to the depth map framebuffer
	// glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
	// depth_map_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
		depth_map_texture, 0);
	glDrawBuffer(GL_NONE);  // disable rendering colors, only write depth values













	// Other camera parameters
	float cameraHorizontalAngle = 90.0f;
	float cameraVerticalAngle = 0.0f;

	// Set projection matrix for shader, this won't change
	mat4 projectionMatrix = perspective(70.0f,            // field of view in degrees
		1024.0f / 768.0f,  // aspect ratio
		0.01f, 200.0f);   // near and far (near > 0)

	glfwSetWindowSizeCallback(window, &windowSizeCallback);

	GLuint projectionMatrixLocation = shaderManager.getUniformLocation("projectionMatrix");
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	// Get the world matrix
	GLuint worldMatrixLocation = shaderManager.getUniformLocation("worldMatrix");

	// For frame time
	float lastFrameTime = glfwGetTime();
	int lastMouseLeftState = GLFW_RELEASE;
	double lastMousePosX, lastMousePosY;
	glfwGetCursorPos(window, &lastMousePosX, &lastMousePosY);

	// Other OpenGL states to set once before the Game Loop
	// Enable Backface culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Enable Depth Test
	glEnable(GL_DEPTH_TEST);

	GLenum renderingMode = GL_TRIANGLES;

	// Track models
	vector<Shape> shapes;               // Set of all shapes in the world
	vector<Wall> walls;					// Set of all walls

	///////// DESIGN MODELS HERE /////////
	// Chi shape
	vector<struct coordinates> chiShape{
		{ 0, 0, 0 },
		{ 0, 0, 1 },
		{ 0, 0, -1 },
		{ 0, 0, 2 },
		{ 0, 0, -2 },
		{ 0, 0, 3 },
		{ 0, 0, -3 },
		{ 1, 0, 0 },
		{ 1, 0, 1 },
		{ 1, 0, -1 },
		{ 1, 0, 2 },
		{ 1, 0, -2 },
		{ -1, 0, 0 },
		{ -1, 0, 1 },
		{ -1, 0, -1 },
		{ -1, 0, 2 },
		{ -1, 0, -2 },
		{ 2, 0, 0 },
		{ 2, 0, 1 },
		{ 2, 0, -1 },
		{ -2, 0, 0 },
		{ -2, 0, 1 },
		{ -2, 0, -1 },
		{ 3, 0, 0 },
		{ -3, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 1, 1 },
		{ 0, 1, -1 },
		{ 0, 1, 2 },
		{ 0, 1, -2 },
		{ 1, 1, 0 },
		{ 1, 1, 1 },
		{ 1, 1, -1 },
		{ -1, 1, 0 },
		{ -1, 1, 1 },
		{ -1, 1, -1 },
		{ 2, 1, 0 },
		{ -2, 1, 0 },
		{ 0, 2, 0 },
		{ 0, 2, 1 },
		{ 0, 2, -1 },
		{ 1, 2, 0 },
		{ -1, 2, 0 },
		{ 0, 3, 0 }
	};

	vector<struct coordinates> alexShape{
		{ 1, 0, 0 },
		{ 1, -1, 0 },
		{ 1, -1, -1 },
		{ 0, -2, -1 },
		{ 1, -2, -1 },
		{ 1, 0, 1 },
		{ 0, 2, 1 },
		{ 0, 1, 1 },
		{ -1, 3, 1 },
		{ -1, 2, 1 },
		{ -1, 3, 0 },
		{ 1, 1, 1 }
	};

	vector<struct coordinates> theoShape{

		{ 0, 0, 0 },
		{ -1, 3, 2 },
		{ -1, 2, 2 },
		{ -1, 1, 2 },
		{ -1, 0, 2 },
		{ -1, -1, 2 },
		{ -1, -2, 2 },
		{ -1, -3, 2 },

		{ 0, 3, 0 },
		{ 1, 3, 0 },
		{ 0, 3, 2 },
		{ 2, 2, 2 },
		{ 2, 1, 2 },
		{ 1, 0, 0 },
		{ 0, -1, 2 },
		{ 1, -2, 2 },
		{ 2, -3, 1 },

		{ -1, 3, 1 },
		{ -1, 3, -1 },
		{ 2, 0, 1 },
		{ 2, -3, -1 },
		{ 1, -3, 0 }
	};

	vector<struct coordinates> antoShape{
		{ 0, 0, 0 },
		{ 0, 0, -1 },
		{ 0, 0, -2 },

		{ 1, 0, 0 },
		{ 1, 0, -1 },
		{ 1, 0, -2 },

		{ 1, 1, 0 },
		{ 1, 1, -1 },
		{ 1, 1, -2 },

		{ 1, -1, 0 },
		{ 1, -1, -1 },
		{ 1, -1, -2 },

		{ -1, 0, 0 },
		{ -1, 0, -1 },
		{ -1, 0, -2 },

		{ -1, 1, -1 },
		{ -1, 1, -2 },
		{ -1, 1, 0 },

		{ -1, -1, 0 },
		{ -1, -1, -1 },
		{ -1, -1, -2 },

		{ 0, -2, -1 },
		{ 0, -2, 0 },
		{ 0, -2, -2 },

		{ 0, 2, 0 },
		{ 0, 2, -1 },
		{ 0, 2, -2 }
	};

	vector<struct coordinates> lightbulbShape{
		{0, 0, 0}
	};

	// Colour of the shapes
	// Chi colour
	int chiColour = createVertexArrayObjectSingleColoured(vec3(0.429f, 0.808f, 0.922f));
	// Alex colour
	int alexColour = createVertexArrayObjectSingleColoured(vec3(0.898f, 0.22f, 0.0f));
	// Theo colour
	int theoColour = createVertexArrayObjectSingleColoured(vec3(1.0f, 0.15f, 0.0f));
	// Anto colour
	int antoColour = createVertexArrayObjectSingleColoured(vec3(0.5f, 0.5f, 0.3f));
	// Lightbulb colour
	int lightbulbColour = createVertexArrayObjectSingleColoured(vec3(1.0f, 1.0f, 1.0f));

	shapes.push_back(Shape(vec3(STAGE_WIDTH, 10.0f, STAGE_WIDTH), chiShape, chiColour, worldMatrixLocation, false, 1.0f));
	shapes.push_back(Shape(vec3(-STAGE_WIDTH, 10.0f, STAGE_WIDTH), alexShape, alexColour, worldMatrixLocation, false, 1.0f));
	shapes.push_back(Shape(vec3(STAGE_WIDTH, 10.0f, -STAGE_WIDTH), theoShape, theoColour, worldMatrixLocation, false, 1.0f));
	shapes.push_back(Shape(vec3(-STAGE_WIDTH, 10.0f, -STAGE_WIDTH), antoShape, antoColour, worldMatrixLocation, false, 1.0f));
	Shape lightbulb = Shape(vec3(0.0f, 0.0f, 0.0f), lightbulbShape, lightbulbColour, worldMatrixLocation, false, 1.0f);

	for (int i = 0; i < 4; i++) {
		walls.push_back(Wall(vec3(shapes[i].mPosition.x, shapes[i].mPosition.y, shapes[i].mPosition.z - WALL_DISTANCE), &(shapes[i]), shapes[i].mvao, worldMatrixLocation));
	}

	int focusedShape = 0;                   // The shape currently being viewed and manipulated
	bool moveCameraToDestination = false;   // Tracks whether the camera is currently moving to a point

	ControlState controlState = { &shapes, &focusedShape };
	glfwSetWindowUserPointer(window, &controlState);

	const vector<vec3> cameraPositions{
		CAMERA_OFFSET + shapes[0].mPosition,
		CAMERA_OFFSET + shapes[1].mPosition,
		CAMERA_OFFSET + shapes[2].mPosition,
		CAMERA_OFFSET + shapes[3].mPosition
	};

	// Camera parameters for view transform
	vec3 cameraPosition = vec3(0.0f, 1.0f, 20.0f);
	vec3 cameraLookAt(0.0f, -1.0f, 0.0f);
	vec3 cameraUp(0.0f, 1.0f, 0.0f);
	vec3 cameraDestination = cameraPosition;
	bool  cameraFirstPerson = true; // press 1 or 2 to toggle this variable
	shaderManager.setVec3("cameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);

	// Set initial view matrix
	mat4 viewMatrix = lookAt(cameraPosition,  // eye
		cameraPosition + cameraLookAt,  // center
		cameraUp); // up

	GLuint viewMatrixLocation = shaderManager.getUniformLocation("viewMatrix");
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);

	// Set up lighting
	vec3 lightPosition = LIGHT_OFFSET;
	shaderManager.setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
	shaderManager.setVec3("lightColour", 1.0f, 1.0f, 1.0f);
	shaderManager.setFloat("ambientLight", LIGHT_AMBIENT_STRENGTH);
	shaderManager.setFloat("diffuseLight", LIGHT_DIFFUSE_STRENGTH);
	shaderManager.setFloat("specularLight", LIGHT_SPECULAR_STRENGTH);
	shaderManager.setFloat("shininess", SHININESS);

	// Grid and coordinate axis colours
	int groundColour = createVertexArrayObjectSingleColoured(vec3(1.0f, 1.0f, 0.0f));
	int xLineColour = createVertexArrayObjectSingleColoured(vec3(1.0f, 0.0f, 0.0f));
	int yLineColour = createVertexArrayObjectSingleColoured(vec3(0.0f, 1.0f, 0.0f));
	int zLineColour = createVertexArrayObjectSingleColoured(vec3(0.0f, 0.0f, 1.0f));

	// Register keypress event callback
	glfwSetKeyCallback(window, &keyCallback);

	// Entering Game Loop
	while (!glfwWindowShouldClose(window))
	{
		// Frame time calculation
		float dt = glfwGetTime() - lastFrameTime;
		lastFrameTime += dt;


		// light parameters
		float Lphi = glfwGetTime() * 0.5f * 3.14f;
		vec3 lightPosition = vec3(0.6f, 50.0f, 5.0f); // the location of the light in 3D space: fixed position
		vec3(cosf(Lphi)* cosf(Lphi), sinf(Lphi),
			-cosf(Lphi) * sinf(Lphi)) *
			5.0f;  // variable position

		vec3 lightFocus(0, 0, -1);  // the point in 3D space the light "looks" at
		vec3 lightDirection = normalize(lightFocus - lightPosition);

		float lightNearPlane = 0.01f;
		float lightFarPlane = 400.0f;

		mat4 lightProjMatrix = //frustum(-1.0f, 1.0f, -1.0f, 1.0f, lightNearPlane, lightFarPlane);
			perspective(50.0f, (float)DEPTH_MAP_TEXTURE_SIZE / (float)DEPTH_MAP_TEXTURE_SIZE, lightNearPlane, lightFarPlane);
		mat4 lightViewMatrix = lookAt(lightPosition, lightFocus, vec3(0, 1, 0));


		SetUniformMat4(shaderScene, "light_proj_view_matrix", lightProjMatrix* lightViewMatrix);
		SetUniform1Value(shaderScene, "light_near_plane", lightNearPlane);
		SetUniform1Value(shaderScene, "light_far_plane", lightFarPlane);
		SetUniformVec3(shaderScene, "light_position", lightPosition);
		SetUniformVec3(shaderScene, "light_direction", lightDirection);

		// DEBUG - MOVING LIGHT
		// TODO: REMOVE BEFORE SUBMISSION
		//float moveY = cos(glfwGetTime());
		//float moveX = sin(glfwGetTime());
		//shaderManager.setVec3("lightPosition", lightPosition.x + moveX * 10.0f - 5.0f, lightPosition.y + moveY * 10.0f - 5.0f, lightPosition.z);

		// Clear Depth Buffer Bit
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw ground
		glBindVertexArray(groundColour);

		for (int i = -50; i <= 50; i++) {
			mat4 gridMatrix = translate(mat4(1.0f), vec3(0.0f, 0.0f, i)) * scale(mat4(1.0f), vec3(100.0f, 0.02f, 0.02f));
			glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &gridMatrix[0][0]);
			glDrawArrays(renderingMode, 0, 36);
		}
		for (int i = -50; i <= 50; i++) {
			mat4 gridMatrix = translate(mat4(1.0f), vec3(i, 0.0f, 0.0f)) * scale(mat4(1.0f), vec3(0.02f, 0.02f, 100.0f));
			glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &gridMatrix[0][0]);
			glDrawArrays(renderingMode, 0, 36);
		}

		// Draw coordinate axes
		glBindVertexArray(xLineColour);
		mat4 xLine = translate(mat4(1.0f), vec3(2.5f, 0.1f, 0.0f)) * scale(mat4(1.0f), vec3(5.0f, 0.1f, 0.1f));
		glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &xLine[0][0]);
		glDrawArrays(renderingMode, 0, 36);
		glBindVertexArray(yLineColour);
		mat4 yLine = translate(mat4(1.0f), vec3(0.0f, 2.6f, 0.0f)) * scale(mat4(1.0f), vec3(0.1f, 5.0f, 0.1f));
		glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &yLine[0][0]);
		glDrawArrays(renderingMode, 0, 36);
		glBindVertexArray(zLineColour);
		mat4 zLine = translate(mat4(1.0f), vec3(0.0f, 0.1f, 2.5f)) * scale(mat4(1.0f), vec3(0.1f, 0.1f, 5.0f));
		glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &zLine[0][0]);
		glDrawArrays(renderingMode, 0, 36);


		for (Shape shape : shapes) {
		

			// Render shadow in 2 passes: 1- Render depth map, 2- Render scene
			// 1- Render shadow map:
			// a- use program for shadows
			// b- resize window coordinates to fix depth map output size
			// c- bind depth map framebuffer to output the depth values
			{
				// Use proper shader
				glUseProgram(shaderShadow);
				// Use proper image output size
				glViewport(0, 0, DEPTH_MAP_TEXTURE_SIZE, DEPTH_MAP_TEXTURE_SIZE);
				// Bind depth map texture as output framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
				// Clear depth data on the framebuffer
				glClear(GL_DEPTH_BUFFER_BIT);
				// Bind geometry
				glBindVertexArray(shape.GetVAO());
				// Draw geometry
				//glDrawElements(GL_TRIANGLES, activeVertices, GL_UNSIGNED_INT, 0);
				glDrawArrays(GL_TRIANGLES, 0, 36);
				// Unbind geometry
				glBindVertexArray(0);
			}

			// 2- Render scene: a- bind the default framebuffer and b- just render like
			// what we do normally
			{
				// Use proper shader
				glUseProgram(shaderScene);
				// Use proper image output size
				// Side note: we get the size from the framebuffer instead of using WIDTH
				// and HEIGHT because of a bug with highDPI displays
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);
				glViewport(0, 0, width, height);
				// Bind screen as output framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// Clear color and depth data on framebuffer
				glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				// Bind depth map texture: not needed, by default it is active
				// glActiveTexture(GL_TEXTURE0);
				// Bind geometry
				glBindVertexArray(shape.GetVAO());
				// Draw geometry
				//glDrawElements(GL_TRIANGLES, activeVertices, GL_UNSIGNED_INT, 0);
				glDrawArrays(GL_TRIANGLES, 0, 36);
				// Unbind geometry
				glBindVertexArray(0);
			}

		}
		/*

		// Draw shape
		for (Shape shape : shapes) {
			shape.Draw(renderingMode);
		}
		lightbulb.mPosition = lightPosition;
		lightbulb.Draw(renderingMode);

		glBindVertexArray(0);

		*/

		// End Frame
		glfwSwapBuffers(window);
		glfwPollEvents();

		// Handle inputs
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		// If shift is held, double camera speed
		bool fastCam = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		float currentCameraSpeed = (fastCam) ? CAMERA_SPEED * 2 : CAMERA_SPEED;

		if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) // FPview
		{
			cameraFirstPerson = true;
		}

		if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) //TPview
		{
			cameraFirstPerson = false;
		}

		double mousePosX, mousePosY;
		glfwGetCursorPos(window, &mousePosX, &mousePosY);

		double dx = mousePosX - lastMousePosX;
		double dy = mousePosY - lastMousePosY;

		lastMousePosX = mousePosX;
		lastMousePosY = mousePosY;

		// Lock the camera rotation to be only when the middle and right button are pressed
		if (lastMouseLeftState == GLFW_RELEASE && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			cameraHorizontalAngle -= dx * CAMERA_ANGULAR_SPEED * dt;
		}
		if (lastMouseLeftState == GLFW_RELEASE && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
			cameraVerticalAngle -= dy * CAMERA_ANGULAR_SPEED * dt;
		}

		if (lastMouseLeftState == GLFW_RELEASE && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			if (!moveCameraToDestination) {
				if (dy < 0) {
					cameraPosition += currentCameraSpeed * cameraLookAt;
				}
				if (dy > 0) {
					cameraPosition -= currentCameraSpeed * cameraLookAt;
				}
				shaderManager.setVec3("cameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
			}
		}

		// Change orientation with the arrow keys
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			cameraFirstPerson = false;
			cameraHorizontalAngle -= CAMERA_ANGULAR_SPEED * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			cameraFirstPerson = false;
			cameraHorizontalAngle += CAMERA_ANGULAR_SPEED * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			cameraFirstPerson = false;
			cameraVerticalAngle -= CAMERA_ANGULAR_SPEED * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			cameraFirstPerson = false;
			cameraVerticalAngle += CAMERA_ANGULAR_SPEED * dt;
		}

		//Go Back to initial position and orientation
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) {
			cameraLookAt.x = 0.0f; cameraLookAt.y = 0.0f; cameraLookAt.z = -1.0f;
			cameraUp.x = 0.0f; cameraUp.y = 1.0f; cameraUp.z = 0.0f;

			//initial orientation
			cameraHorizontalAngle = 90.0f;
			cameraVerticalAngle = 0.0f;

			cameraDestination = vec3(0.0f, 1.0f, 20.0f);
			lightPosition = cameraDestination + LIGHT_OFFSET;
			shaderManager.setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
			moveCameraToDestination = true;
		}

		// Clamp vertical angle to [-85, 85] degrees
		cameraVerticalAngle = std::max(-85.0f, std::min(85.0f, cameraVerticalAngle));
		if (cameraHorizontalAngle > 360)
		{
			cameraHorizontalAngle -= 360;
		}
		else if (cameraHorizontalAngle < -360)
		{
			cameraHorizontalAngle += 360;
		}

		float theta = radians(cameraHorizontalAngle);
		float phi = radians(cameraVerticalAngle);

		cameraLookAt = vec3(cosf(phi) * cosf(theta), sinf(phi), -cosf(phi) * sinf(theta));
		vec3 cameraSideVector = cross(cameraLookAt, vec3(0.0f, 1.0f, 0.0f));
		normalize(cameraSideVector);

		// Select shapes via 1-4 keys
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		{
			focusedShape = 0;
			cameraDestination = cameraPositions[0];
			lightPosition = cameraDestination + LIGHT_OFFSET;
			moveCameraToDestination = true;
			shaderManager.setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
		}
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		{
			focusedShape = 1;
			cameraDestination = cameraPositions[1];
			lightPosition = cameraDestination + LIGHT_OFFSET;
			moveCameraToDestination = true;
			shaderManager.setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
		}
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		{
			focusedShape = 2;
			cameraDestination = cameraPositions[2];
			lightPosition = cameraDestination + LIGHT_OFFSET;
			moveCameraToDestination = true;
			shaderManager.setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
		}
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		{
			focusedShape = 3;
			cameraDestination = cameraPositions[3];
			lightPosition = cameraDestination + LIGHT_OFFSET;
			moveCameraToDestination = true;
			shaderManager.setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
		}

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Ry
		{
			shapes[focusedShape].mOrientation.y += ROTATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // R-y
		{
			shapes[focusedShape].mOrientation.y -= ROTATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) // Rz
		{
			shapes[focusedShape].mOrientation.z += ROTATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) // R-z
		{
			shapes[focusedShape].mOrientation.z -= ROTATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) // Rx
		{
			shapes[focusedShape].mOrientation.x += ROTATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) // R-x
		{
			shapes[focusedShape].mOrientation.x -= ROTATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) // grow object
		{
			shapes[focusedShape].mScale += SCALE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) // shrink object
		{
			shapes[focusedShape].mScale -= SCALE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // move object forward
		{
			shapes[focusedShape].mPosition.z -= TRANSLATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // move object backward
		{
			shapes[focusedShape].mPosition.z += TRANSLATE_RATE * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // move object left
		{
			shapes[focusedShape].mPosition.x -= TRANSLATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // move object right
		{
			shapes[focusedShape].mPosition.x += TRANSLATE_RATE * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) // Set triangle rendering mode
		{
			renderingMode = GL_TRIANGLES;
		}

		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) // Set line rendering mode
		{
			renderingMode = GL_LINE_LOOP;
		}

		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) // Set point rendering mode
		{
			renderingMode = GL_POINTS;
		}

		// Reshuffle shape
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		{
			shapes[focusedShape].Reshuffle();
		}
		// Reset shape position and orientation
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			shapes[focusedShape].ResetPosition();
		}

		if (moveCameraToDestination) {
			// Slide the camera towards its destination
			vec3 cameraDelta = cameraDestination - cameraPosition;
			if (abs(cameraDelta.x) < 0.1 && abs(cameraDelta.y) < 0.1 && abs(cameraDelta.z) < 0.1) {
				// We have arrived at the destination
				cameraPosition = cameraDestination;
				moveCameraToDestination = false;
			}
			cameraPosition += cameraDelta * CAMERA_JUMP_SPEED * dt;

			// Reset camera orientation
			cameraLookAt.x = 0.0f; cameraLookAt.y = 0.0f; cameraLookAt.z = -1.0f;
			cameraUp.x = 0.0f; cameraUp.y = 1.0f; cameraUp.z = 0.0f;
			cameraHorizontalAngle = 90.0f;
			cameraVerticalAngle = 0.0f;
			shaderManager.setVec3("cameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
		}

		//Antonio's part
		mat4 viewMatrix = mat4(1.0);

		vec3 position = cameraPosition;
		viewMatrix = lookAt(position, position + cameraLookAt, cameraUp);
		//if camera is third person, approximate the radius and give the world orientation perspective aimed at the origin {0,0,0}, Press N to normalize view to first person
		if (cameraFirstPerson) {
			viewMatrix = lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);
		}
		else {
			int newx;
			int newy;
			int newz;
			if (cameraPosition.x < 0) {
				newx = cameraPosition.x * -1;
			}
			else
				newx = cameraPosition.x;
			if (cameraPosition.y < 0)
			{
				newy = cameraPosition.y * -1;
			}
			else
				newy = cameraPosition.y;
			if (cameraPosition.z < 0)
			{
				newz = cameraPosition.z * -1;
			}
			else
				newz = cameraPosition.z;
			float radius = sqrt(pow(newx, 2) + pow(newy, 2) + pow(newz, 2));
			vec3 position = vec3{ 0,1,0 } -radius * cameraLookAt;
			viewMatrix = lookAt(position, position + cameraLookAt, cameraUp);
			shaderManager.setVec3("cameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
		}
		glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);

	}

	// Shutdown GLFW
	glfwTerminate();

	return 0;
}

int createVertexArrayObjectColoured(vec3 frontBackColour, vec3 topBottomColour, vec3 leftRightColour)
{
	// Cube model
	// Vertex, colour, normal
	vec3 unitCube[] = {
		vec3(-0.5f,-0.5f,-0.5f), leftRightColour, vec3(-1.0f, 0.0f, 0.0f), // left
		vec3(-0.5f,-0.5f, 0.5f), leftRightColour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f, 0.5f, 0.5f), leftRightColour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f,-0.5f,-0.5f), leftRightColour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f, 0.5f, 0.5f), leftRightColour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f, 0.5f,-0.5f), leftRightColour, vec3(-1.0f, 0.0f, 0.0f),

		vec3(0.5f, 0.5f,-0.5f), frontBackColour, vec3(0.0f, 0.0f, -1.0f), // far
		vec3(-0.5f,-0.5f,-0.5f), frontBackColour, vec3(0.0f, 0.0f, -1.0f),
		vec3(-0.5f, 0.5f,-0.5f), frontBackColour, vec3(0.0f, 0.0f, -1.0f),
		vec3(0.5f, 0.5f,-0.5f), frontBackColour, vec3(0.0f, 0.0f, -1.0f),
		vec3(0.5f,-0.5f,-0.5f), frontBackColour, vec3(0.0f, 0.0f, -1.0f),
		vec3(-0.5f,-0.5f,-0.5f), frontBackColour, vec3(0.0f, 0.0f, -1.0f),

		vec3(0.5f,-0.5f, 0.5f), topBottomColour, vec3(0.0f, -1.0f, 0.0f), // bottom
		vec3(-0.5f,-0.5f,-0.5f), topBottomColour, vec3(0.0f, -1.0f, 0.0f),
		vec3(0.5f,-0.5f,-0.5f), topBottomColour, vec3(0.0f, -1.0f, 0.0f),
		vec3(0.5f,-0.5f, 0.5f), topBottomColour, vec3(0.0f, -1.0f, 0.0f),
		vec3(-0.5f,-0.5f, 0.5f), topBottomColour, vec3(0.0f, -1.0f, 0.0f),
		vec3(-0.5f,-0.5f,-0.5f), topBottomColour, vec3(0.0f, -1.0f, 0.0f),

		vec3(-0.5f, 0.5f, 0.5f), frontBackColour, vec3(0.0f, 0.0f, 1.0f), // near
		vec3(-0.5f,-0.5f, 0.5f), frontBackColour, vec3(0.0f, 0.0f, 1.0f),
		vec3(0.5f,-0.5f, 0.5f), frontBackColour, vec3(0.0f, 0.0f, 1.0f),
		vec3(0.5f, 0.5f, 0.5f), frontBackColour, vec3(0.0f, 0.0f, 1.0f),
		vec3(-0.5f, 0.5f, 0.5f), frontBackColour, vec3(0.0f, 0.0f, 1.0f),
		vec3(0.5f,-0.5f, 0.5f), frontBackColour, vec3(0.0f, 0.0f, 1.0f),

		vec3(0.5f, 0.5f, 0.5f), leftRightColour, vec3(1.0f, 0.0f, 0.0f), // right
		vec3(0.5f,-0.5f,-0.5f), leftRightColour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f, 0.5f,-0.5f), leftRightColour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f,-0.5f,-0.5f), leftRightColour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f, 0.5f, 0.5f), leftRightColour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f,-0.5f, 0.5f), leftRightColour, vec3(1.0f, 0.0f, 0.0f),

		vec3(0.5f, 0.5f, 0.5f), topBottomColour, vec3(0.0f, 1.0f, 0.0f), // top
		vec3(0.5f, 0.5f,-0.5f), topBottomColour, vec3(0.0f, 1.0f, 0.0f),
		vec3(-0.5f, 0.5f,-0.5f), topBottomColour, vec3(0.0f, 1.0f, 0.0f),
		vec3(0.5f, 0.5f, 0.5f), topBottomColour, vec3(0.0f, 1.0f, 0.0f),
		vec3(-0.5f, 0.5f,-0.5f), topBottomColour, vec3(0.0f, 1.0f, 0.0f),
		vec3(-0.5f, 0.5f, 0.5f), topBottomColour, vec3(0.0f, 1.0f, 0.0f)
	};

	// Create a vertex array
	GLuint vertexArrayObject;
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	// Upload Vertex Buffer to the GPU, keep a reference to it (vertexBufferObject)
	GLuint vertexBufferObject;
	glGenBuffers(1, &vertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(unitCube), unitCube, GL_STATIC_DRAW);

	glVertexAttribPointer(0,    // Location 0 matches aPos in Vertex Shader
		3,						// size
		GL_FLOAT,				// type
		GL_FALSE,				// normalized?
		3 * sizeof(vec3),		// stride
		(void*)0				// array buffer offset
	);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1,	// Location 1 matches aColor in Vertex Shader
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(vec3),
		(void*)(sizeof(vec3))
	);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2,	// Location 2 matches aNormal in Vertex Shader
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(vec3),
		(void*)(sizeof(vec3) * 2)
	);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	return vertexArrayObject;
}

int createVertexArrayObjectSingleColoured(vec3 colour)
{
	// Cube model
	// Vertex, colour, normal
	vec3 unitCube[] = {
		vec3(-0.5f,-0.5f,-0.5f), colour, vec3(-1.0f, 0.0f, 0.0f), // left
		vec3(-0.5f,-0.5f, 0.5f), colour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f, 0.5f, 0.5f), colour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f,-0.5f,-0.5f), colour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f, 0.5f, 0.5f), colour, vec3(-1.0f, 0.0f, 0.0f),
		vec3(-0.5f, 0.5f,-0.5f), colour, vec3(-1.0f, 0.0f, 0.0f),

		vec3(0.5f, 0.5f,-0.5f), colour, vec3(0.0f, 0.0f, -1.0f), // far
		vec3(-0.5f,-0.5f,-0.5f), colour, vec3(0.0f, 0.0f, -1.0f),
		vec3(-0.5f, 0.5f,-0.5f), colour, vec3(0.0f, 0.0f, -1.0f),
		vec3(0.5f, 0.5f,-0.5f), colour, vec3(0.0f, 0.0f, -1.0f),
		vec3(0.5f,-0.5f,-0.5f), colour, vec3(0.0f, 0.0f, -1.0f),
		vec3(-0.5f,-0.5f,-0.5f), colour, vec3(0.0f, 0.0f, -1.0f),

		vec3(0.5f,-0.5f, 0.5f), colour, vec3(0.0f, -1.0f, 0.0f), // bottom
		vec3(-0.5f,-0.5f,-0.5f), colour, vec3(0.0f, -1.0f, 0.0f),
		vec3(0.5f,-0.5f,-0.5f), colour, vec3(0.0f, -1.0f, 0.0f),
		vec3(0.5f,-0.5f, 0.5f), colour, vec3(0.0f, -1.0f, 0.0f),
		vec3(-0.5f,-0.5f, 0.5f), colour, vec3(0.0f, -1.0f, 0.0f),
		vec3(-0.5f,-0.5f,-0.5f), colour, vec3(0.0f, -1.0f, 0.0f),

		vec3(-0.5f, 0.5f, 0.5f), colour, vec3(0.0f, 0.0f, 1.0f), // near
		vec3(-0.5f,-0.5f, 0.5f), colour, vec3(0.0f, 0.0f, 1.0f),
		vec3(0.5f,-0.5f, 0.5f), colour, vec3(0.0f, 0.0f, 1.0f),
		vec3(0.5f, 0.5f, 0.5f), colour, vec3(0.0f, 0.0f, 1.0f),
		vec3(-0.5f, 0.5f, 0.5f), colour, vec3(0.0f, 0.0f, 1.0f),
		vec3(0.5f,-0.5f, 0.5f), colour, vec3(0.0f, 0.0f, 1.0f),

		vec3(0.5f, 0.5f, 0.5f), colour, vec3(1.0f, 0.0f, 0.0f), // right
		vec3(0.5f,-0.5f,-0.5f), colour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f, 0.5f,-0.5f), colour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f,-0.5f,-0.5f), colour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f, 0.5f, 0.5f), colour, vec3(1.0f, 0.0f, 0.0f),
		vec3(0.5f,-0.5f, 0.5f), colour, vec3(1.0f, 0.0f, 0.0f),

		vec3(0.5f, 0.5f, 0.5f), colour, vec3(0.0f, 1.0f, 0.0f), // top
		vec3(0.5f, 0.5f,-0.5f), colour, vec3(0.0f, 1.0f, 0.0f),
		vec3(-0.5f, 0.5f,-0.5f), colour, vec3(0.0f, 1.0f, 0.0f),
		vec3(0.5f, 0.5f, 0.5f), colour, vec3(0.0f, 1.0f, 0.0f),
		vec3(-0.5f, 0.5f,-0.5f), colour, vec3(0.0f, 1.0f, 0.0f),
		vec3(-0.5f, 0.5f, 0.5f), colour, vec3(0.0f, 1.0f, 0.0f)
	};

	// Create a vertex array
	GLuint vertexArrayObject;
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	// Upload Vertex Buffer to the GPU, keep a reference to it (vertexBufferObject)
	GLuint vertexBufferObject;
	glGenBuffers(1, &vertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(unitCube), unitCube, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0,    // Location 0 matches aPos in Vertex Shader
		3,						// size
		GL_FLOAT,				// type
		GL_FALSE,				// normalized?
		3 * sizeof(vec3),		// stride
		(void*)0				// array buffer offset
	);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1,	// Location 1 matches aColor in Vertex Shader
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(vec3),
		(void*)(sizeof(vec3))
	);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2,	// Location 2 matches aNormal in Vertex Shader
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(vec3),
		(void*)(sizeof(vec3) * 2)
	);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	return vertexArrayObject;
}

bool initContext() {     // Initialize GLFW and OpenGL version
	glfwInit();

#if defined(PLATFORM_OSX)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
	// On windows, we set OpenGL version to 2.1, to support more hardware
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#endif

	// Create Window and rendering context using GLFW, resolution is 1024x768
	window = glfwCreateWindow(1024, 768, "COMP 371 - Assignment 1 by Spiral Staircase", NULL, NULL);
	if (window == NULL)
	{
		cerr << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	// The next line disables the mouse cursor
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// 
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		cerr << "Failed to create GLEW" << endl;
		glfwTerminate();
		return false;
	}
	return true;
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	GLint shaderProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shaderProgram);

	mat4 projectionMatrix = glm::perspective(70.0f,            // field of view in degrees
		(float)width / (float)height,  // aspect ratio
		0.01f, 200.0f);   // near and far (near > 0)

	GLuint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	ControlState controlState = *(ControlState*)glfwGetWindowUserPointer(window);

	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		controlState.shapes->at(*(controlState.focusedShape)).mPosition.z -= TRANSLATE_RATE * 0.2;
	}
	if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		controlState.shapes->at(*(controlState.focusedShape)).mPosition.z += TRANSLATE_RATE * 0.2;
	}
}
