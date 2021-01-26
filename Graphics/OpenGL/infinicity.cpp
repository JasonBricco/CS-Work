// Jason Bricco (jmbricco)
// 10/24/2020
// CS4611 - Infinicity

#define NOMINMAX

#include "libkuhl.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <vector>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace std;

static bool debugCam = false;
static bool goForward = false;
static bool goBackward = false;

// 2D vector structure.
struct Vec2
{
	float x, y;

	Vec2() : x(0.0f), y(0.0f) {}
	Vec2(float v) : x(v), y(v) {}
	Vec2(float x, float y) : x(x), y(y) {}
};

static Vec2 operator *(Vec2 v, float s)
{
	return Vec2(v.x * s, v.y * s);
}

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

static Vec3 operator /(Vec3 v, float s)
{
	return Vec3(v.x / s, v.y / s, v.z / s);
}

static Vec3 operator *(Vec3 v, float s)
{
	return Vec3(v.x * s, v.y * s, v.z * s);
}

static Vec3 operator +(Vec3 a, Vec3 b)
{
	return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static Vec3 operator -(Vec3 a, Vec3 b)
{
	return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

typedef Vec3 Color;

// Stores vertex/index data for drawing objects. For efficiency, I do not
// use dynamically growing vectors and instead choose a vertex count that
// is more than the maximum that can be required by the city.
struct Mesh
{
	uint32_t vertCount = 0;
	Vec3 vertices[131072];

	uint32_t uvCount = 0;
	Vec2 uvs[131072];

	uint32_t colorCount = 0;
	Color colors[131072];

	uint32_t indexCount = 0;
	uint32_t indices[196608];

	Mesh() {}

	void AddIndices()
	{
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount + 1;
		indices[indexCount++] = vertCount;

		indices[indexCount++] = vertCount + 3;
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount;
	}

	void AddColoredVertex(Vec3 vert, Color color)
	{
		vertices[vertCount++] = vert;
		colors[colorCount++] = color;
	}

	void AddTexturedVertex(Vec3 vert, Vec2 uv)
	{
		vertices[vertCount++] = vert;
		uvs[uvCount++] = uv;
	}

	// Fills a kuhl_geometry object using the data from this mesh.
	void GetGeometry(kuhl_geometry* geom, GLuint program)
	{
		kuhl_geometry_new(geom, program, vertCount, GL_TRIANGLES);
		kuhl_geometry_attrib(geom, &vertices[0].x, 3, "in_Position", 1);

		if (colorCount > 0)
			kuhl_geometry_attrib(geom, &colors[0].x, 3, "in_Color", 1);

		if (uvCount > 0)
			kuhl_geometry_attrib(geom, &uvs[0].x, 2, "in_TexCoord", 1);

		kuhl_geometry_indices(geom, indices, indexCount);
	}

	// Effectively clears the mesh and allows it to be reused for a 
	// modified city.
	void Reset()
	{
		vertCount = 0;
		uvCount = 0;
		colorCount = 0;
		indexCount = 0;
	}
};

struct Camera
{
	Vec3 pos;
	float viewMat[16];

	// Set the view matrix based on the camera's current position,
	// looking toward positive z and slightly down.
	void UpdateViewMatrix()
	{
		float camP[3] = { pos.x, pos.y, pos.z };
		float camLook[3] = { pos.x, pos.y - 0.5f, pos.z + 1.0f };
		float camUp[3] = { 0.0f, 1.0f, 0.0f };
		mat4f_lookatVec_new(viewMat, camP, camLook, camUp);
	}

	// Moves the camera to the given location.
	void MoveTo(float x, float y, float z)
	{
		pos = Vec3(x, y, z);
		UpdateViewMatrix();
	}

	// Moves the camera by the given amount relative
	// to its current location.
	void MoveBy(Vec3 amt)
	{
		pos += amt;
		UpdateViewMatrix();
	}
};

// Represents a building in the city.
struct Building
{
	Vec3 pos, size;

	// The number of windows in each dimension. This is used
	// to compute the size of the building so that windows
	// fit properly.
	int windowsX, windowsY, windowsZ;

	bool complex;

	// Padding between windows and between the edge and first/last windows.
	float edgePad = 0.4f;
	float innerPad = 0.15f;
};

// Represents the city, comprised of buildings and the roads.
struct City
{
	// Size and count for city blocks.
	float cellSize;
	int cellCount;

	// The block the camera is currently in.
	int currentBlock = 1;

	GLuint roadTex;
	GLuint cityShader, groundShader;

	bool hasBuildings;
	kuhl_geometry buildings;

	Vec3 groundP;
	kuhl_geometry ground;

	// Mesh for storing building vertex data.
	Mesh* mesh;
};

// GLFW keyboard callback function for receiving key presses.
static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	/* If the library handles this keypress, return */
	if (kuhl_keyboard_handler(window, key, scancode, action, mods))
		return;

	if (action == GLFW_PRESS)
	{
		// Change car or wheel angles from A, S, D, F keys, held or pressed.
		if (key == GLFW_KEY_F1)
			debugCam = !debugCam;
	}

	if (key == GLFW_KEY_SPACE)
		goForward = action == GLFW_PRESS || action == GLFW_REPEAT;

	if (key == GLFW_KEY_B)
		goBackward = action == GLFW_PRESS || action == GLFW_REPEAT;
}

// Generates a random number between min and max, inclusive.
static int RandomRange(int min, int max)
{
	return min + rand() % ((max + 1) - min);
}

// Generates a random true/false value.
static bool RandomBool()
{
	return RandomRange(0, 1) == 0;
}

// Functions for adding a colored quad, used for buildings and windows.

static void MakeQuadUp(Mesh* mesh, Vec3 pos, Vec3 size, Color color)
{
	mesh->AddIndices();
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y + size.y, pos.z), color);
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y + size.y, pos.z + size.z), color);
	mesh->AddColoredVertex(Vec3(pos.x, pos.y + size.y, pos.z + size.z), color);
	mesh->AddColoredVertex(Vec3(pos.x, pos.y + size.y, pos.z), color);
}

static void MakeQuadBack(Mesh* mesh, Vec3 pos, Vec3 size, Color color)
{
	mesh->AddIndices();
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y, pos.z), color);
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y + size.y, pos.z), color);
	mesh->AddColoredVertex(Vec3(pos.x, pos.y + size.y, pos.z), color);
	mesh->AddColoredVertex(Vec3(pos.x, pos.y, pos.z), color);
}

static void MakeQuadRight(Mesh* mesh, Vec3 pos, Vec3 size, Color color)
{
	mesh->AddIndices();
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y, pos.z + size.z), color);
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y + size.y, pos.z + size.z), color);
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y + size.y, pos.z), color);
	mesh->AddColoredVertex(Vec3(pos.x + size.x, pos.y, pos.z), color);
}

static void MakeQuadLeft(Mesh* mesh, Vec3 pos, Vec3 size, Color color)
{
	mesh->AddIndices();
	mesh->AddColoredVertex(Vec3(pos.x, pos.y, pos.z), color);
	mesh->AddColoredVertex(Vec3(pos.x, pos.y + size.y, pos.z), color);
	mesh->AddColoredVertex(Vec3(pos.x, pos.y + size.y, pos.z + size.z), color);
	mesh->AddColoredVertex(Vec3(pos.x, pos.y, pos.z + size.z), color);
}

static void MakeCube(Mesh* mesh, Vec3 pos, Vec3 size, Color color)
{
	MakeQuadUp(mesh, pos, size, color);
	MakeQuadBack(mesh, pos, size, color);
	MakeQuadRight(mesh, pos, size, color);
	MakeQuadLeft(mesh, pos, size, color);
}

// Given a number of windows, uses the padding to calculate the total size
// of the building necessary to fit those windows.
static float SizeFromWindowCount(int windows, float edgePad, float innerPad)
{
	return windows + (innerPad * (windows - 1)) + (edgePad * 2.0f);
}

static void SizeFromWindowCount(Building& building)
{
	float edgePad = building.edgePad, innerPad = building.innerPad;

	float width = SizeFromWindowCount(building.windowsX, edgePad, innerPad);
	float height = SizeFromWindowCount(building.windowsY, edgePad, innerPad);
	float depth = SizeFromWindowCount(building.windowsZ, edgePad, innerPad);

	building.size = Vec3(width, height, depth);
}

// Adds the building walls and windows to the mesh.
static void GenerateBuilding(Mesh* mesh, Building* building, Camera* cam)
{
	Color gray = Color(0.4f, 0.4f, 0.4f);
	MakeCube(mesh, building->pos, building->size, gray);

	float windowThickness = 0.05f;

	// Add windows to the back side.
	for (int x = 0; x < building->windowsX; ++x)
	{
		for (int y = 0; y < building->windowsY; ++y)
		{
			float locX = building->edgePad + x + (x * building->innerPad);
			float locY = building->edgePad + y + (y * building->innerPad);

			Vec3 windowP = building->pos + Vec3(locX, locY, -windowThickness);

			Vec3 color = RandomRange(0, 2) == 0 ? Vec3(1.0f, 1.0f, 0.0f) : Vec3(0.0f);
			MakeQuadBack(mesh, windowP, Vec3(1.0f, 1.0f, windowThickness), color);
		}
	}

	// Add windows to the left and right sides.
	for (int z = 0; z < building->windowsZ; ++z)
	{
		for (int y = 0; y < building->windowsY; ++y)
		{
			float locZ = building->edgePad + z + (z * building->innerPad);
			float locY = building->edgePad + y + (y * building->innerPad);

			Vec3 color = RandomRange(0, 2) == 0 ? Vec3(1.0f, 1.0f, 0.0f) : Vec3(0.0f);

			// Mirror the windows the left and right sides. Only one of these will
			// ever be seen, depending on which side of the camera it is on. I only
			// draw the windows if they will be seen, as an optimization.
			Vec3 windowP = building->pos + Vec3(-windowThickness, locY, locZ);

			if (windowP.x > cam->pos.x)
				MakeQuadLeft(mesh, windowP, Vec3(windowThickness, 1.0f, 1.0f), color);

			windowP = building->pos + Vec3(building->size.x, locY, locZ);

			if (windowP.x < cam->pos.x)
				MakeQuadRight(mesh, windowP, Vec3(windowThickness, 1.0f, 1.0f), color);
		}
	}
}

// Creates a new building at the specified position. The building's size
// and window configuration are randomly computed.
static void MakeBuilding(Mesh* mesh, Camera* cam, Vec3 pos)
{
	int windowMinH = 4, windowMaxH = 6;
	int windowMinV = 8, windowMaxV = 12;

	Building building = { pos };
	building.windowsX = RandomRange(windowMinH, windowMaxH);
	building.windowsY = RandomRange(windowMinV, windowMaxV);
	building.windowsZ = RandomRange(windowMinH, windowMaxH);
	building.complex = RandomBool();

	SizeFromWindowCount(building);
	GenerateBuilding(mesh, &building, cam);
	
	if (building.complex)
	{
		// Add another building above this one in the case of a complex building.
		Building upper = {};
		upper.windowsX = RandomRange(std::max(building.windowsX - 3, 2), building.windowsX - 1);
		upper.windowsY = RandomRange(windowMinV, windowMaxV);
		upper.windowsZ = RandomRange(std::max(building.windowsZ - 3, 2), building.windowsZ - 1);

		SizeFromWindowCount(upper);

		upper.pos = (building.pos + building.size * 0.5f) - (upper.size * 0.5f);
		upper.pos.y = building.pos.y + building.size.y;

		GenerateBuilding(mesh, &upper, cam);
	}
}

// Starting at a specified z location, generates a cellCount * cellCount
// area of buildings and gets a geometry object from it. 
// Erases the existing city area in order to build a new one.
static void CreateBuildings(City* city, Camera* cam, int startZ)
{
	if (city->hasBuildings)
		kuhl_geometry_delete(&city->buildings);

	for (int z = startZ; z < startZ + city->cellCount; ++z)
	{
		for (int x = 0; x < city->cellCount; ++x)
		{
			int hash = z * 31 + x;
			srand(hash);

			Vec3 pos = Vec3(x, 0.0f, z) * city->cellSize;
			pos.x += 2.0f;
			pos.z += 1.0f;
			MakeBuilding(city->mesh, cam, pos);
		}
	}

	city->mesh->GetGeometry(&city->buildings, city->cityShader);
	city->hasBuildings = true;

	city->mesh->Reset();
}

// Generates the ground plane and the initial city buildings.
// Sets up the city state.
static void GenerateCity(City* city, Camera* cam)
{
	city->cellSize = 10.0f;
	city->cellCount = 10;

	Mesh* groundMesh = new Mesh();
	Vec2 groundSize = Vec2(city->cellSize) * city->cellCount;

	groundMesh->AddIndices();

	// To tile the road texture, scale uvs based on the cell count.
	groundMesh->AddTexturedVertex(Vec3(groundSize.x, 0.0f, 0.0f), Vec2(city->cellCount, 0.0f));
	groundMesh->AddTexturedVertex(Vec3(groundSize.x, 0.0f, groundSize.y), Vec2(city->cellCount));
	groundMesh->AddTexturedVertex(Vec3(0.0f, 0.0f, groundSize.y), Vec2(0.0f, city->cellCount));
	groundMesh->AddTexturedVertex(Vec3(0.0f, 0.0f, 0.0f), Vec2(0.0f, 0.0f));

	groundMesh->GetGeometry(&city->ground, city->groundShader);
	delete groundMesh;

	kuhl_geometry_texture(&city->ground, city->roadTex, "tex", 1);

	cam->MoveTo(groundSize.x * 0.5f + 1.0f, 20.0f, city->cellSize);

	city->mesh = new Mesh();
	CreateBuildings(city, cam, 0);
}

// Sets the given shader to be used by OpenGL and sends the modelView/projection
// matrices to the driver.
static void SetShader(GLuint shader, float* modelView, float* proj)
{
	glUseProgram(shader);

	glUniformMatrix4fv(kuhl_get_uniform("Projection"), 1, 0, proj);
	glUniformMatrix4fv(kuhl_get_uniform("ModelView"), 1, 0, modelView);
}

static void Render(City* city, Camera* cam)
{
	viewmat_begin_frame();

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float debugView[16], perspective[16];
	viewmat_get(debugView, perspective, 0);

	float* view = debugCam ? debugView : cam->viewMat;

	// Draw the ground.
	float groundModel[16];
	mat4f_translate_new(groundModel, city->groundP.x, city->groundP.y, city->groundP.z);

	float groundModelView[16];
	mat4f_mult_mat4f_new(groundModelView, view, groundModel);

	SetShader(city->groundShader, groundModelView, perspective);
	kuhl_geometry_draw(&city->ground);

	// Draw the buildings.
	SetShader(city->cityShader, view, perspective);
	kuhl_geometry_draw(&city->buildings);

	viewmat_end_frame();
	kuhl_errorcheck();
}

// Used for per-frame updates (camera movement and city rebuilding).
static void Update(City* city, Camera* cam, float frameTime)
{
	const float speed = 10.0f;

	Vec3 move = Vec3(0.0f);

	if (goForward)
		move.z += speed * frameTime;
	
	if (goBackward)
		move.z -= speed * frameTime;

	cam->MoveBy(move);

	int block = (int)floorf(cam->pos.z / 10.0f);

	// Set the ground location to some multiple of the cell size 
	// so that the roads don't appear to move.
	city->groundP.z = (block - 1) * city->cellSize;

	// If the camera enters a new block, regenerate the city buildings based
	// on the new location.
	if (block != city->currentBlock)
	{
		CreateBuildings(city, cam, block - 1);
		city->currentBlock = block;
	}
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
	glCullFace(GL_BACK);

	City city = {};

	kuhl_read_texture_file_wrap("../Images/Road.png", &city.roadTex, GL_REPEAT, GL_REPEAT);

	city.cityShader = kuhl_create_program("city.vert", "city.frag");
	kuhl_errorcheck();

	city.groundShader = kuhl_create_program("ground.vert", "ground.frag");
	kuhl_errorcheck();

	Camera cam = {};
	GenerateCity(&city, &cam);
	
	// Create the debug view matrix.
	float debugCamP[3] = { 0.0f, 0.0f, -10.0f };
	float debugCamLook[3] = { 0.0f, 0.0f, 0.0f };
	float debugCamUp[3] = { 0.0f, 1.0f, 0.0f };

	viewmat_init(debugCamP, debugCamLook, debugCamUp);

	float frameTime = 0.0f;
	float lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(kuhl_get_window()))
	{
		glfwPollEvents();

		Update(&city, &cam, frameTime);

		Render(&city, &cam);
		kuhl_errorcheck();

		// Compute time elapsed during the frame to use during
		// keyboard input, in order to make the values go from
		// per frame to per second.
		float endTime = glfwGetTime();
		frameTime = endTime - lastTime;
		lastTime = endTime;
	}
}
