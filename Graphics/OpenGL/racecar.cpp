// Jason Bricco
// CS4611 OpenGL 1
// 9/29/20

#include "libkuhl.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Globals.
static GLuint program = 0;
static float frameTime = 0.0f; // How much time the last frame took.
static float carAngle = 0.0f; // Whole-car rotation angle.
static float wheelOffsetAngle = 0.0f; // Angle for rotating wheels side to side.
static bool wheelsStopped = false; // Whether or not wheels are spinning.

// 3-component vector structure.
struct Vec3
{
	float values[3];

	Vec3() { memset(values, 0, sizeof(values)); }

	Vec3(float x, float y, float z)
	{
		values[0] = x;
		values[1] = y;
		values[2] = z;
	}

	float x() { return values[0]; }
	float y() { return values[1]; }
	float z() { return values[2]; }
};

// Function pointer to tell DrawObject() how to make a model matrix.
using ModelFunc = void(*)(float* mat, Vec3 pos, Vec3 scale);

typedef Vec3 Color;

static Vec3 operator *(Vec3 a, Vec3 b)
{
	return Vec3(a.x() * b.x(), a.y() * b.y(), a.z() + b.z());
}

static Vec3 operator *(Vec3 a, float s)
{
	return Vec3(a.x() * s, a.y() * s, a.z() * s);
}

static float Dot(Vec3 a, Vec3 b)
{
	Vec3 tmp(a * b);
	return tmp.x() + tmp.y();
}

static float InverseSqrt(float v)
{
	return 1.0f / sqrtf(v);
}

static Vec3 Normalize(Vec3 v)
{
	return v * InverseSqrt(Dot(v, v));
}

// Cube vertex definitions.
static Vec3 cubeUp[4] =
{
	Vec3(+0.5f, +0.5f, -0.5f),
	Vec3(+0.5f, +0.5f, +0.5f),
	Vec3(-0.5f, +0.5f, +0.5f),
	Vec3(-0.5f, +0.5f, -0.5f)
};

static Vec3 cubeDown[4] =
{
	Vec3(-0.5f, -0.5f, -0.5f),
	Vec3(-0.5f, -0.5f, +0.5f),
	Vec3(+0.5f, -0.5f, +0.5f),
	Vec3(+0.5f, -0.5f, -0.5f)
};

static Vec3 cubeFront[4] =
{
	Vec3(-0.5f, -0.5f, +0.5f),
	Vec3(-0.5f, +0.5f, +0.5f),
	Vec3(+0.5f, +0.5f, +0.5f),
	Vec3(+0.5f, -0.5f, +0.5f)
};

static Vec3 cubeBack[4] =
{
	Vec3(+0.5f, -0.5f, -0.5f),
	Vec3(+0.5f, +0.5f, -0.5f),
	Vec3(-0.5f, +0.5f, -0.5f),
	Vec3(-0.5f, -0.5f, -0.5f)
};

static Vec3 cubeRight[4] =
{
	Vec3(+0.5f, -0.5f, +0.5f),
	Vec3(+0.5f, +0.5f, +0.5f),
	Vec3(+0.5f, +0.5f, -0.5f),
	Vec3(+0.5f, -0.5f, -0.5f)
};

static Vec3 cubeLeft[4] =
{
	Vec3(-0.5f, -0.5f, -0.5f),
	Vec3(-0.5f, +0.5f, -0.5f),
	Vec3(-0.5f, +0.5f, +0.5f),
	Vec3(-0.5f, -0.5f, +0.5f)
};

// Ensures value v is between min and max.
static float Clamp(float v, float min, float max)
{
	return (v < min) ? min : (max < v) ? max : v;
}

// Allow car angle to range between 0 and 360.
static void ChangeCarAngle(float& angle, float sign, float speed)
{
	angle += (frameTime * sign) * speed;

	if (angle < 0.0f)
		angle += 360.0f;
}

// Allow wheel angle to range between -45 and 45.
static void ChangeWheelAngle(float& angle, float sign, float speed)
{
	angle = Clamp(angle + (frameTime * sign) * speed, -45.0f, 45.0f);
}

// GLFW keyboard callback function for receiving key presses.
static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/* If the library handles this keypress, return */
	if (kuhl_keyboard_handler(window, key, scancode, action, mods))
		return;

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		// Change car or wheel angles from A, S, D, F keys, held or pressed.
		if (key == GLFW_KEY_D) ChangeCarAngle(carAngle, -1.0f, 180.0f);
		if (key == GLFW_KEY_F) ChangeCarAngle(carAngle, 1.0f, 180.0f);
		if (key == GLFW_KEY_A) ChangeWheelAngle(wheelOffsetAngle, -1.0f, 180.0f);
		if (key == GLFW_KEY_S) ChangeWheelAngle(wheelOffsetAngle, 1.0f, 180.0f);
	}
	
	// Only toggle spinning on a single press.
	if (action == GLFW_PRESS && key == GLFW_KEY_SPACE)
		wheelsStopped = !wheelsStopped;
}

// Stores vertex/index data and allows manipulation of it.
struct Mesh
{
	int vertCount = 0;
	Vec3* vertices;
	Vec3* normals;
	Color* colors;

	int indexCount = 0;
	uint32_t* indices;

	float* VertexData()
	{
		return &vertices[0].values[0];
	}

	uint32_t* IndexData()
	{
		return indices;
	}

	float* NormalData()
	{
		return &normals[0].values[0];
	}

	float* ColorData()
	{
		return &colors[0].values[0];
	}

	Mesh()
	{
		vertices = new Vec3[256];
		normals = new Vec3[256];
		colors = new Color[256];
		indices = new uint32_t[256 / 4 * 6];
	}

	// Add a quad (set of 4 vertices with 2 triangles). If computeNorm is true,
	// normals will be computed based on the direction vector from origin,
	// since the car is centered around the origin.
	void AddQuad(Vec3* verts, Color color, Vec3 norm, bool computeNorm = false)
	{
		// Add quad indices.
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount + 1;
		indices[indexCount++] = vertCount;

		indices[indexCount++] = vertCount + 3;
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount;
			
		// Add given quad vertices, colors, and normals.
		for (int i = 0; i < 4; ++i)
		{
			int dataIndex = vertCount + i;

			vertices[dataIndex] = verts[i];

			if (computeNorm)
			{
				norm = Normalize(Vec3(verts[i].x(), verts[i].y(), 0.0f));
				normals[dataIndex] = norm;
			}
			else normals[dataIndex] = norm;

			colors[dataIndex] = color;
		}

		vertCount += 4;
	}

	// Add a triangle (set of 3 vertices) to the mesh.
	void AddTriangle(Vec3* verts, Color color, Vec3 norm)
	{
		// Add triangle indices.
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount + 1;
		indices[indexCount++] = vertCount;

		// Add triangle vertices, normals, and colors.
		for (int i = 0; i < 3; ++i)
		{
			int dataIndex = vertCount + i;

			vertices[dataIndex] = verts[i];
			normals[dataIndex] = norm;
			colors[dataIndex] = color;
		}

		vertCount += 3;
	}

	// Add a single vertex to the mesh.
	void AddVertex(Vec3 vert, Vec3 norm, Color color)
	{
		vertices[vertCount] = vert;
		normals[vertCount] = norm;
		colors[vertCount] = color;

		++vertCount;
	}

	// Add a single index to the mesh.
	void AddIndex(uint32_t index)
	{
		indices[indexCount++] = vertCount + index;
	}

	~Mesh()
	{
		delete[] vertices;
		delete[] indices;
		delete[] normals;
		delete[] colors;
	}
};

// Creates a cube and sends its data to the GPU.
static void CreateCube(kuhl_geometry* cube)
{
	Color carColor = Color(0.05f, 0.2f, 0.66f);

	Mesh mesh = Mesh();

	mesh.AddQuad(cubeUp, carColor, Vec3(0.0f, 1.0f, 0.0f));
	mesh.AddQuad(cubeDown, carColor, Vec3(0.0f, -1.0f, 0.0f));
	mesh.AddQuad(cubeFront, carColor, Vec3(0.0f, 0.0f, 1.0f));
	mesh.AddQuad(cubeBack, carColor, Vec3(0.0f, 0.0f, -1.0f));
	mesh.AddQuad(cubeRight, carColor, Vec3(1.0f, 0.0f, 0.0f));
	mesh.AddQuad(cubeLeft, carColor, Vec3(-1.0f, 0.0f, 0.0f));

	kuhl_geometry_new(cube, program, mesh.vertCount, GL_TRIANGLES);

	kuhl_geometry_attrib(cube, mesh.VertexData(), 3, "in_Position", 1);
	kuhl_geometry_attrib(cube, mesh.NormalData(), 3, "in_Normal", 1);
	kuhl_geometry_attrib(cube, mesh.ColorData(), 3, "in_Color", 1);
	kuhl_geometry_indices(cube, mesh.IndexData(), mesh.indexCount);
}

// Creates a hexagonal face of a wheel, storing its data in the mesh object.
static void CreateWheelFace(Mesh* mesh, Color color, float z, Vec3* vertList)
{
	Vec3 norm = Vec3(0.0f, 0.0f, z < 0.0f ? -1.0f : 1.0f);
	
	// For each face/triangle on bottom, what are the 3 vertices?
	for (int i = 0; i < 6; i++)
	{
		mesh->AddIndex(0);
		mesh->AddIndex(i + 1);

		if (i + 2 >= 7)
			mesh->AddIndex(1);
		else mesh->AddIndex(i + 2);
	}

	mesh->AddVertex(Vec3(0.0f, 0.0f, z), norm, color);

	// For each vertex around bottom perimeter
	for (int i = 0; i < 6; ++i)
	{
		Vec3 vert = Vec3(0.5f * sinf(i * M_PI / 3.0f), 0.5f * cosf(i * M_PI / 3.0f), z);
		mesh->AddVertex(vert, norm, color);

		vertList[i] = vert;
	}
}

// Adds a quad for a wheel edge to the mesh object, based on the given front and back vertices.
static void WheelEdgeQuad(Mesh* mesh, Color color, Vec3* front, Vec3* back, int startIndex)
{
	Vec3 vertsToAdd[4];

	int endIndex = (startIndex + 1) % 6;

	vertsToAdd[0] = back[startIndex];
	vertsToAdd[1] = front[startIndex];
	vertsToAdd[2] = front[endIndex];
	vertsToAdd[3] = back[endIndex];

	mesh->AddQuad(vertsToAdd, color, Vec3(), true);
}

// Creates a hexagonal approximated wheel cylinder and sends its
// data to the GPU.
static void CreateWheel(kuhl_geometry* wheel)
{
	// Dark gray.
	Color color = Color(0.65f, 0.65f, 0.65f);

	Vec3 frontVertices[6];
	Vec3 backVertices[6];

	Mesh mesh = Mesh();
	CreateWheelFace(&mesh, color, -0.25f, frontVertices);
	CreateWheelFace(&mesh, color, +0.25f, backVertices);

	for (int i = 0; i < 6; ++i)
		WheelEdgeQuad(&mesh, color, frontVertices, backVertices, i);

	kuhl_geometry_new(wheel, program, mesh.vertCount, GL_TRIANGLES);

	kuhl_geometry_attrib(wheel, mesh.VertexData(), 3, "in_Position", 1);
	kuhl_geometry_attrib(wheel, mesh.NormalData(), 3, "in_Normal", 1);
	kuhl_geometry_attrib(wheel, mesh.ColorData(), 3, "in_Color", 1);
	kuhl_geometry_indices(wheel, mesh.IndexData(), mesh.indexCount);
}

// Function for computing the model matrix for the car's cubes.
static void CarModel(float* model, Vec3 pos, Vec3 scale)
{
	// Moves the object to the given location.
	float translateMat[16];
	mat4f_translate_new(translateMat, pos.x(), pos.y(), pos.z());

	// Scales the object to the given size.
	float scaleMat[16];
	mat4f_scale_new(scaleMat, scale.x(), scale.y(), scale.z());

	float rotateMat[16];
	mat4f_rotateAxis_new(rotateMat, carAngle, 0.0f, 1.0f, 0.0f);

	// Translate first, then rotate so that the cubes rotate around
	// a world space axis instead of object space.
	mat4f_mult_mat4f_new(model, rotateMat, translateMat);
	mat4f_mult_mat4f_new(model, model, scaleMat);
}

// Function for computing a model matrix for a wheel object.
static void WheelModel(float* model, Vec3 pos, Vec3 scale)
{
	float translateMat[16];
	mat4f_translate_new(translateMat, pos.x(), pos.y(), pos.z());

	float scaleMat[16];
	mat4f_scale_new(scaleMat, scale.x(), scale.y(), scale.z());
	
	float wheelRotate[16];

	float yWheelRotate[16];
	mat4f_rotateAxis_new(yWheelRotate, wheelOffsetAngle, 0.0f, 1.0f, 0.0f);

	// Angle of rotation for spinning. It is marked static so that its 
	// value is preserved across the program's duration. 
	static float zAngle = 0.0f;

	if (!wheelsStopped)
	{
		// Increment the angle only when wheels are spinning.
		zAngle += frameTime * 15.0f;

		if (zAngle > 360.0f)
			zAngle = 0.0f;
	}

	float zWheelRotate[16];
	mat4f_rotateAxis_new(zWheelRotate, zAngle, 0.0f, 0.0f, 1.0f);
	mat4f_mult_mat4f_new(wheelRotate, yWheelRotate, zWheelRotate);

	float yCarRotate[16];
	mat4f_rotateAxis_new(yCarRotate, carAngle, 0.0f, 1.0f, 0.0f);

	// Wheel rotation has two separate parts. There is a rotation in
	// object space for wheel tilting and spinning (wheelRotate). This 
	// happens when rotation would normally happen - directly after scaling
	// and before translating. I use another rotation matrix (yCarRotate) after
	// translating so the wheels can rotate around a world space
	// axis like the rest of the car, to stay with the car when it spins.
	mat4f_mult_mat4f_new(model, yCarRotate, translateMat);
	mat4f_mult_mat4f_new(model, model, wheelRotate);
	mat4f_mult_mat4f_new(model, model, scaleMat);
}

// Computes the model matrix for a geometry object using modelFunc as well as
// the modelview and normal matrices, and submits the uniforms/draw call to the GPU.
static void DrawObject(kuhl_geometry* obj, ModelFunc modelFunc, Vec3 pos, Vec3 scale, float* view)
{
	float model[16];
	modelFunc(model, pos, scale);
	
	float modelview[16];
	mat4f_mult_mat4f_new(modelview, view, model);

	/* Send the modelview matrix to the vertex program. */
	glUniformMatrix4fv(kuhl_get_uniform("ModelView"), 1, 0, modelview);

	float normalMat[9];
	mat3f_from_mat4f(normalMat, modelview);
	mat3f_invert(normalMat);
	mat3f_transpose(normalMat);

	glUniformMatrix3fv(kuhl_get_uniform("NormalMat"), 1, 0, normalMat);

	kuhl_geometry_draw(obj);
}

// Draw the entire car.
static void Render(kuhl_geometry* cube, kuhl_geometry* wheel)
{
	viewmat_begin_frame();

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Get the view matrix and the projection matrix */
	float viewMat[16], perspective[16];
	viewmat_get(viewMat, perspective, 0);

	/* Send the perspective projection matrix to the vertex program. */
	glUniformMatrix4fv(kuhl_get_uniform("Projection"), 1, 0, perspective);

	// Draw car body.
	DrawObject(cube, CarModel, Vec3(-2.0f, +0.5f, +0.0f), Vec3(2.0f, 1.0f, 1.5f), viewMat);
	DrawObject(cube, CarModel, Vec3(+0.0f, +1.0f, +0.0f), Vec3(2.0f, 2.0f, 1.5f), viewMat);
	DrawObject(cube, CarModel, Vec3(+2.0f, +0.5f, +0.0f), Vec3(2.0f, 1.0f, 1.5f), viewMat);

	// Draw front wheels.
	DrawObject(wheel, WheelModel, Vec3(-1.0f, +0.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f), viewMat);
	DrawObject(wheel, WheelModel, Vec3(-1.0f, +0.0f, +1.0f), Vec3(1.0f, 1.0f, 1.0f), viewMat);

	// Draw back wheels.
	DrawObject(wheel, WheelModel, Vec3(+1.0f, +0.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f), viewMat);
	DrawObject(wheel, WheelModel, Vec3(+1.0f, +0.0f, +1.0f), Vec3(1.0f, 1.0f, 1.0f), viewMat);

	viewmat_end_frame();
	kuhl_errorcheck();
}

int main(int argc, char** argv)
{
	/* Initialize GLFW and GLEW */
	kuhl_ogl_init(&argc, argv, 512, 512, 32, 4);

	/* Specify function to call when keys are pressed. */
	glfwSetKeyCallback(kuhl_get_window(), OnKey);

	/* Compile and link a GLSL program composed of a vertex shader and
	 * a fragment shader. */
	program = kuhl_create_program("triangle-shade.vert", "triangle-shade.frag");

	/* Use the GLSL program so subsequent calls to glUniform*() send the variable to
	   the correct program. */
	glUseProgram(program);
	kuhl_errorcheck();

	kuhl_geometry cube;
	CreateCube(&cube);

	kuhl_geometry wheel;
	CreateWheel(&wheel);

	// Create the view matrix.
	float camP[3] = { 0.0f, 0.0f, 10.0f };
	float camLook[3] = { 0.0f, 0.0f, 0.0f };
	float camUp[3] = { 0.0f, 1.0f, 0.0f };
	
	viewmat_init(camP, camLook, camUp);

	int viewport[4]; 
	viewmat_get_viewport(viewport, 0);

	/* Tell OpenGL the area of the window that we will be drawing in. */
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
	
	// Set background color.
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

	glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
	
	kuhl_errorcheck();

	float lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(kuhl_get_window()))
	{
		/* Process events (keyboard, mouse, etc) */
		glfwPollEvents();

		Render(&cube, &wheel);
		kuhl_errorcheck();
		
		// Compute time elapsed during the frame to use during
		// keyboard input, in order to make the values go from
		// per frame to per second.
		float endTime = glfwGetTime();
		frameTime = endTime - lastTime;
		lastTime = endTime;
	}
	
	exit(EXIT_SUCCESS);
}
