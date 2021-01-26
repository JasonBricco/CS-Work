#include "libkuhl.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// 2D vector structure.
struct Vec2
{
	float x, y;

	Vec2() : x(0.0f), y(0.0f) {}
	Vec2(float x, float y) : x(x), y(y) {}
};

// 3D vector structure.
struct Vec3
{
	float x, y, z;

	Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
	Vec3(float v) : x(v), y(v), z(v) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

// Used to determine the function to compute the model matrix for an object.
using ModelFunc = void(*)(float* mat, Vec3 pos, Vec3 scale, float angle);

// Stores information needed for drawing textured quads.
// Includes the texture handle, rotation angle, model
// view matrix for sorting, and distance from the camera.
struct Quad
{
	kuhl_geometry obj;
	GLuint tex;
	float angle;
	float modelView[16];
	float zDistFromCamera;
};

// Information used in Render() for displaying objects.
struct RenderData
{
	kuhl_geometry* model;
	std::vector<Quad> quads;
	GLuint texShader, modelShader;
	float camZ;
};

// Stores vertex/index data for drawing objects.
struct Mesh
{
	int vertCount = 0;
	Vec3* vertices;
	Vec2* uvs;

	int indexCount = 0;
	uint32_t* indices;

	Mesh(int vertCount)
	{
		vertices = new Vec3[vertCount];
		uvs = new Vec2[vertCount];
		indices = new uint32_t[(int64_t)vertCount / 4 * 6];
	}

	Mesh() : Mesh(256) {}

	// Data pointers for sending data to OpenGL.
	float* VertexData()
	{
		return &vertices[0].x;
	}

	float* UvData()
	{
		return &uvs[0].x;
	}

	uint32_t* IndexData()
	{
		return indices;
	}

	// Adds vertices and uvs for a quad into the mesh.
	void AddQuad(Vec3* vertsToAdd, Vec2* uvsToAdd)
	{
		// Add quad indices.
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount + 1;
		indices[indexCount++] = vertCount;

		indices[indexCount++] = vertCount + 3;
		indices[indexCount++] = vertCount + 2;
		indices[indexCount++] = vertCount;

		for (int i = 0; i < 4; ++i)
		{
			int dataIndex = vertCount + i;

			vertices[dataIndex] = vertsToAdd[i];
			uvs[dataIndex] = uvsToAdd[i];
		}

		vertCount += 4;
	}

	~Mesh()
	{
		delete[] vertices;
		delete[] uvs;
		delete[] indices;
	}
};

// Draws the duck model, centered on origin.
static void DrawModel(kuhl_geometry* modelObj, float* viewMat)
{
	float model[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-0.081f, -0.525f, 0.022f, 1.0f
	};

	float modelView[16];
	mat4f_mult_mat4f_new(modelView, viewMat, model);

	glUniformMatrix4fv(kuhl_get_uniform("ModelView"), 1, 0, modelView);
	kuhl_geometry_draw(modelObj);
}

// Model matrix for the textured quad objects.
static void QuadModel(float* model, Vec3 pos, Vec3 scale, float angle)
{
	// Moves the object to the given location.
	float translateMat[16];
	mat4f_translate_new(translateMat, pos.x, pos.y, pos.z);

	// Scales the object to the given size.
	float scaleMat[16];
	mat4f_scale_new(scaleMat, scale.x, scale.y, scale.z);

	float rotateMat[16];
	mat4f_rotateAxis_new(rotateMat, angle, 0.0f, 1.0f, 0.0f);

	// Translate first, then rotate so that the cubes rotate around
	// a world space axis instead of object space.
	mat4f_mult_mat4f_new(model, rotateMat, translateMat);
	mat4f_mult_mat4f_new(model, model, scaleMat);
}

static void ComputeModelView(float* modelView, ModelFunc getModel, Vec3 pos, Vec3 scale, float angle, float* view)
{
	float model[16];
	getModel(model, pos, scale, angle);

	mat4f_mult_mat4f_new(modelView, view, model);
}

static void Render(RenderData* data, float frameTime)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	viewmat_begin_frame();

	int viewport[4];
	viewmat_get_viewport(viewport, 0);

	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);

	float viewMat[16], perspective[16];
	viewmat_get(viewMat, perspective, 0);

	glUseProgram(data->modelShader);
	kuhl_errorcheck();

	// Set the projection matrix to the model shader.
	glUniformMatrix4fv(kuhl_get_uniform("Projection"), 1, 0, perspective);

	DrawModel(data->model, viewMat);

	glUseProgram(data->texShader);
	kuhl_errorcheck();
	
	// Set the projection matrix to the texture shader.
	glUniformMatrix4fv(kuhl_get_uniform("Projection"), 1, 0, perspective);

	// Go through each quad, computing the model-view matrix and camera distance for each.
	for (Quad& quad : data->quads)
	{
		quad.angle += frameTime * 45.0f;

		if (quad.angle > 360.0f)
			quad.angle = 0.0f;

		Vec3 pos = Vec3(0.0f, 0.0f, -3.0f);
		Vec3 scale = Vec3(2.0f, 2.0f, 1.0f);

		ComputeModelView(quad.modelView, QuadModel, pos, scale, quad.angle, viewMat);

		// Transform the position into camera coordinates by multiplying by
		// the model-view matrix. Then, we can compute the correct distance
		// from the camera.
		float pVec[4] = { pos.x, pos.y, pos.z, 1.0f };
		mat4f_mult_vec4f(pVec, quad.modelView);

		quad.zDistFromCamera = fabsf(pVec[2] - data->camZ);
	}

	// Sort the array based on distance from the camera, with 
	// farther away objects in front of closer ones.
	std::sort(data->quads.begin(), data->quads.end(), [](auto a, auto b)
	{
		return a.zDistFromCamera > b.zDistFromCamera;
	});

	// Draw each quad.
	for (Quad& quad : data->quads)
	{
		glUniformMatrix4fv(kuhl_get_uniform("ModelView"), 1, 0, quad.modelView);
		kuhl_geometry_draw(&quad.obj);
	}

	viewmat_end_frame();
}

// Creates a quad object with the given aspect ratio (width / height).
static void CreateQuad(kuhl_geometry* quad, GLuint tex, GLuint program, float aspect)
{
	Mesh mesh = Mesh(4);

	aspect *= 0.5f;

	Vec3 verts[4] =
	{
		Vec3(+aspect, -0.5f, -0.5f),
		Vec3(+aspect, +0.5f, -0.5f),
		Vec3(-aspect, +0.5f, -0.5f),
		Vec3(-aspect, -0.5f, -0.5f)
	};

	Vec2 uvs[4] =
	{
		Vec2(1.0f, 0.0f),
		Vec2(1.0f, 1.0f),
		Vec2(0.0f, 1.0f),
		Vec2(0.0f, 0.0f)
	};

	mesh.AddQuad(verts, uvs);

	kuhl_geometry_new(quad, program, mesh.vertCount, GL_TRIANGLES);

	kuhl_geometry_attrib(quad, mesh.VertexData(), 3, "in_Position", 1);
	kuhl_geometry_attrib(quad, mesh.UvData(), 2, "in_TexCoord", 1);
	kuhl_geometry_indices(quad, mesh.IndexData(), mesh.indexCount);

	kuhl_geometry_texture(quad, tex, "tex", 1);
}

int main(int argc, char** argv)
{
	// Initialize OpenGL and enable necessary state.
	kuhl_ogl_init(&argc, argv, 512, 512, 32, 4);

	GLuint modelShader = kuhl_create_program("carousel.vert", "carousel.frag");
	kuhl_errorcheck();

	kuhl_geometry* model = kuhl_load_model("../models/duck/duck.dae", NULL, modelShader, NULL);

	GLuint texShader = kuhl_create_program("texture.vert", "texture.frag");
	kuhl_errorcheck();

	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	
	// Create the view matrix.
	float camP[3] = { 0.0f, 0.0f, 10.0f };
	float camLook[3] = { 0.0f, 0.0f, 0.0f };
	float camUp[3] = { 0.0f, 1.0f, 0.0f };
	viewmat_init(camP, camLook, camUp);

	if (argc == 1)
	{
		printf("No images provided to load.\n");
		return -1;
	}

	RenderData data = {};
	data.model = model;
	data.texShader = texShader;
	data.modelShader = modelShader;
	data.camZ = camP[2];

	// Each quad should be 'angleDiff' angles apart.
	// Depends on how many quads we're drawing.
	float angleDiff = 360.0f / (argc - 1);

	for (int i = 1; i < argc; ++i)
	{
		Quad quad;

		// Load specified textures (from command arguments), and 
		// create quads using the texture information.
		float aspect = kuhl_read_texture_file(argv[i], &quad.tex);

		if (aspect < 0.0f)
		{
			printf("Failed to load image: %s\n", argv[i]);
			return -1;
		}

		CreateQuad(&quad.obj, quad.tex, texShader, aspect);
		quad.angle = ((i - 1) * angleDiff);
		data.quads.push_back(quad);
	}

	float frameTime = 0.0f;
	float lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(kuhl_get_window()))
	{
		glfwPollEvents();

		Render(&data, frameTime);
		kuhl_errorcheck();

		// Compute time elapsed during the frame to use during
		// keyboard input, in order to make the values go from
		// per frame to per second.
		float endTime = glfwGetTime();
		frameTime = endTime - lastTime;
		lastTime = endTime;
	}

	return 0;
}
