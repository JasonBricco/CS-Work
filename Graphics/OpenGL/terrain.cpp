// Jason Bricco (jmbricco)
// CS4611 Terrain Assignment
// 11/14/20

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#include "libkuhl.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Globals.
// If true, the default mouse-based camera movement occurs.
// Otherwise, the camera moves around the map automatically.
static bool manualCam = false;

// 2D vector structure.
struct Vec2
{
	float x, y;

	Vec2() : x(0.0f), y(0.0f) {}
	Vec2(float v) : x(v), y(v) {}
	Vec2(float x, float y) : x(x), y(y) {}
};

// 3D vector structure.
struct Vec3
{
	float x, y, z;

	Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
	Vec3(float v) : x(v), y(v), z(v) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	Vec3& operator +=(Vec3 other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}
};

// Vector math operators.
static Vec3 operator +(Vec3 a, Vec3 b)
{
	return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static Vec3 operator -(Vec3 a, Vec3 b)
{
	return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static Vec3 operator /(Vec3 a, float s)
{
	return Vec3(a.x / s, a.y / s, a.z / s);
}

static Vec3 operator *(Vec3 a, Vec3 b)
{
	return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

static bool operator ==(Vec3 a, Vec3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

static float Dot(Vec3 a, Vec3 b)
{
	Vec3 tmp(a * b);
	return tmp.x + tmp.y + tmp.z;
}

static float Length(Vec3 v)
{
	return sqrtf(Dot(v, v));
}

// Moves current toward target by 'dist' amount,
// without overshooting the target. Returns the new
// position, not a relative offset.
static Vec3 MoveTowards(Vec3 current, Vec3 target, float dist)
{
	Vec3 dir = target - current;

	float len = Length(dir);

	if (len <= dist || len == 0.0f)
		return target;

	return current + (dir / len) * dist;
}

// Stores vertex/index data for drawing objects. For efficiency, I do not
// use dynamically growing vectors and instead choose a vertex count that
// is more than the maximum that can be required by the city.
struct Mesh
{
	uint32_t vertCount = 0;
	Vec3 vertices[2097152];

	uint32_t uvCount = 0;
	Vec2 uvs[2097152];

	uint32_t indexCount = 0;
	uint32_t indices[16777216];

	void AddVertex(Vec3 vert, Vec2 uv)
	{
		vertices[vertCount++] = vert;
		uvs[uvCount++] = uv;
	}

	// Add indices for a single triangle.
	void AddIndices(int a, int b, int c)
	{
		indices[indexCount++] = a;
		indices[indexCount++] = b;
		indices[indexCount++] = c;
	}

	void AddQuadIndices()
	{
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount + 1;
		indices[indexCount++] = vertCount;

		indices[indexCount++] = vertCount + 3;
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount;
	}

	// Fills a kuhl_geometry object using the data from this mesh.
	void GetGeometry(kuhl_geometry* geom, GLuint program)
	{
		kuhl_geometry_new(geom, program, vertCount, GL_TRIANGLES);
		kuhl_geometry_attrib(geom, &vertices[0].x, 3, "in_Position", 1);
		kuhl_geometry_attrib(geom, &uvs[0].x, 2, "in_TexCoord", 1);
		kuhl_geometry_indices(geom, indices, indexCount);
	}
};

struct Camera
{
	Vec3 pos;
	float viewMat[16];
	
	// The position the camera is looking at.
	Vec3 lookAtP;

	// A list of positions the camera can move through.
	// targetP is the index for the current target.
	int targetP;
	Vec3 positions[10];

	// Set the view matrix based on the camera's current position,
	// looking toward positive z and slightly down.
	void UpdateViewMatrix()
	{
		float camP[3] = { pos.x, pos.y, pos.z };
		float camLook[3] = { lookAtP.x, lookAtP.y, lookAtP.z };
		float camUp[3] = { 0.0f, 1.0f, 0.0f };
		mat4f_lookatVec_new(viewMat, camP, camLook, camUp);
	}

	// Moves the camera to the given location.
	void MoveTo(float x, float y, float z)
	{
		pos = Vec3(x, y, z);
		UpdateViewMatrix();
	}

	void MoveTo(Vec3 v)
	{
		MoveTo(v.x, v.y, v.z);
	}

	// Moves the camera by the given amount relative
	// to its current location.
	void MoveBy(Vec3 amt)
	{
		pos += amt;
		UpdateViewMatrix();
	}
};

// Represents the terrain layer.
struct Terrain
{
	kuhl_geometry geom;
	GLuint heightmap, color;
	GLuint shader;
	int sizeX, sizeZ;
};

// Represents the cloud layer.
struct Clouds
{
	kuhl_geometry geom;
	GLuint texture;
	GLuint shader;
};

// GLFW keyboard callback function for receiving key presses.
static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/* If the library handles this keypress, return */
	if (kuhl_keyboard_handler(window, key, scancode, action, mods))
		return;

	if (action == GLFW_PRESS && key == GLFW_KEY_SPACE)
		manualCam = !manualCam;
}

// Sets the given shader to be used by OpenGL and sends the modelView/projection
// matrices to the driver.
static void SetShader(GLuint shader, float* modelView, float* proj)
{
	glUseProgram(shader);

	glUniformMatrix4fv(kuhl_get_uniform("Projection"), 1, 0, proj);
	glUniformMatrix4fv(kuhl_get_uniform("ModelView"), 1, 0, modelView);
}

static void Render(Camera* cam, Terrain* terrain, Clouds* clouds)
{
	viewmat_begin_frame();

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float debugView[16], perspective[16];
	viewmat_get(debugView, perspective, 0);

	float* view = manualCam ? debugView : cam->viewMat;

	// Shift the terrain so it is centered on the origin.
	float offsetX = -terrain->sizeX * 0.25f;
	float offsetZ = -terrain->sizeZ * 0.25f;

	float model[16];
	mat4f_translate_new(model, offsetX, 0.0f, offsetZ);

	float modelView[16];
	mat4f_mult_mat4f_new(modelView, view, model);

	SetShader(terrain->shader, modelView, perspective);
	kuhl_geometry_draw(&terrain->geom);

	// Enable alpha blending for cloud drawing so that we can see through it.
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);

	SetShader(clouds->shader, modelView, perspective);
	kuhl_geometry_draw(&clouds->geom);

	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	viewmat_end_frame();
	kuhl_errorcheck();
}

// Moves the camera through the scene if in
// automatic camera movement mode.
static void Update(Camera* cam, float frameTime)
{
	if (!manualCam)
	{
		const float speed = 100.0f;

		Vec3 target = cam->positions[cam->targetP];
		Vec3 move = MoveTowards(cam->pos, target, speed * frameTime);

		cam->MoveTo(move.x, move.y, move.z);

		if (move == target)
			cam->targetP = (cam->targetP + 1) % (int)ArrayCount(cam->positions);
	}
}

static void CreateTerrainQuad(Terrain* terrain)
{
	Mesh* mesh = new Mesh();

	int sizeX = terrain->sizeX;
	int sizeZ = terrain->sizeZ;

	// Generate terrain vertices.
	for (int z = 0; z < sizeZ; ++z)
	{
		for (int x = 0; x < sizeX; ++x)
		{
			Vec3 vert = Vec3(x * 0.5f, 0.0f, z * 0.5f);

			float uvX = 1.0f - ((float)x / (sizeX - 1));
			float uvY = (float)z / (sizeZ - 1);

			Vec2 uv = Vec2(uvX, uvY);
			mesh->AddVertex(vert, uv);
		}
	}

	// Generate terrain indices.
	int i = 0;

	for (int y = 0; y < sizeZ - 1; ++y)
	{
		for (int x = 0; x < sizeX; ++x)
		{
			if (x != sizeX - 1)
			{
				mesh->AddIndices(i, i + 1, i + 1 + sizeX);
				mesh->AddIndices(i + sizeX, i, i + 1 + sizeX);
			}

			++i;
		}
	}

	mesh->GetGeometry(&terrain->geom, terrain->shader);

	kuhl_geometry_texture(&terrain->geom, terrain->heightmap, "heightmap", 1);
	kuhl_geometry_texture(&terrain->geom, terrain->color, "color", 1);

	delete mesh;
}

static void CreateClouds(Clouds* clouds)
{
	float sizeX = 1024.0f, sizeZ = 512.0f;

	Mesh* mesh = new Mesh();

	// Generate clouds as a single quad that covers the map size.
	mesh->AddQuadIndices();
	mesh->AddVertex(Vec3(sizeX, 100.0f, 0.0f), Vec2(1.0f, 0.0f));
	mesh->AddVertex(Vec3(sizeX, 100.0f, sizeZ), Vec2(1.0f, 1.0f));
	mesh->AddVertex(Vec3(0.0f, 100.0f, sizeZ), Vec2(0.0f, 1.0f));
	mesh->AddVertex(Vec3(0.0f, 100.0f, 0.0f), Vec2(0.0f, 0.0f));

	mesh->GetGeometry(&clouds->geom, clouds->shader);

	kuhl_geometry_texture(&clouds->geom, clouds->texture, "tex", 1);

	delete mesh;
}

// Loads the texture from disk and returns a handle to it.
static GLuint LoadTexture(const char* name)
{
	GLuint tex = 0;
	float aspect = kuhl_read_texture_file(name, &tex);

	if (aspect < 0.0f)
	{
		printf("Failed to load image: %s\n", name);
		return -1;
	}

	return tex;
}

int main(int argc, char** argv)
{
	// Initialize and set OpenGL state.
	kuhl_ogl_init(&argc, argv, 512, 512, 32, 4);

	glfwSetKeyCallback(kuhl_get_window(), OnKey);

	glClearColor(0.12f, 0.12f, 0.12f, 0.0f);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	Terrain terrain = {};
	terrain.sizeX = 2048;
	terrain.sizeZ = 1024;

	terrain.shader = kuhl_create_program("terrain.vert", "terrain.frag");
	kuhl_errorcheck();

	terrain.heightmap = LoadTexture("../images/topo.png");
	terrain.color = LoadTexture("../images/color.png");

	CreateTerrainQuad(&terrain);

	Clouds clouds = {};
	clouds.shader = kuhl_create_program("clouds.vert", "clouds.frag");
	clouds.texture = LoadTexture("../images/clouds.png");

	CreateClouds(&clouds);

	Vec3 startP = Vec3(220.0f, 10.0f, 200.0f);

	Camera cam = {};

	// Create the debug view matrix.
	float debugCamP[3] = { startP.x, startP.y, startP.z };
	float debugCamLook[3] = { 0.0f, 0.0f, 0.0f };
	float debugCamUp[3] = { 0.0f, 1.0f, 0.0f };

	viewmat_init(debugCamP, debugCamLook, debugCamUp);

	int viewport[4];
	viewmat_get_viewport(viewport, 0);

	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	cam.lookAtP = Vec3(0.0f, 10.0f, 0.0f);

	// Fill the list of positions the camera can move through.
	cam.positions[0] = Vec3(-500.0f, 40.0f, -245.0f);
	cam.positions[1] = Vec3(-500.0f, 40.0f, 245.0f);
	cam.positions[2] = Vec3(500.0f, 20.0f, 245.0f);
	cam.positions[3] = Vec3(500.0f, 40.0f, -245.0f);
	cam.positions[4] = Vec3(0.0f, 80.0f, -245.0f);
	cam.positions[5] = Vec3(-500.0f, 40.0f, -245.0f);
	cam.positions[6] = Vec3(-300.0f, 120.0f, -150.0f);
	cam.positions[7] = Vec3(300.0f, 120.0f, -150.0f);
	cam.positions[8] = Vec3(300.0f, 120.0f, 150.0f);
	cam.positions[9] = Vec3(-300.0f, 120.0f, 150.0f);

	cam.MoveTo(cam.positions[0]);

	float frameTime = 0.0f;
	float lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(kuhl_get_window()))
	{
		glfwPollEvents();

		Update(&cam, frameTime);

		Render(&cam, &terrain, &clouds);
		kuhl_errorcheck();

		// Compute time elapsed during the frame to use during
		// keyboard input, in order to make the values go from
		// per frame to per second.
		float endTime = glfwGetTime();
		frameTime = endTime - lastTime;
		lastTime = endTime;
	}
}
