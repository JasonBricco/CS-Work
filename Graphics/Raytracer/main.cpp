//
// Jason Bricco
// CS4611 - Assignment 3
// 9/16/20
// 

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdint.h>
#include <float.h>

#define ArrayCount(array) ((int)(sizeof(array) / sizeof((array)[0])))

constexpr int IMAGE_PIXEL_SIZE = 512;
constexpr float IMAGE_UNIT_SIZE = 2.0f;
constexpr float UNIT_PIXEL = IMAGE_UNIT_SIZE / IMAGE_PIXEL_SIZE;
constexpr float HALF_UNIT_PIXEL = UNIT_PIXEL * 0.5f;

constexpr int CHANNELS = 3;
constexpr int BYTES_PER_PIXEL = 3;

constexpr float AMBIENT = 0.2f;
constexpr float EPSILON = 0.001f;

// 3D vector structure.
struct Vec3
{
	float x, y, z;

	Vec3() {}
	Vec3(float v) : x(v), y(v), z(v) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	void Print()
	{
		printf("(%.2f, %.2f, %.2f)\n", x, y, z);
	}
};

// Overload operators to allow using standard math syntax with vectors.
static Vec3 operator *(Vec3 a, Vec3 b)
{
    return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

static Vec3 operator *(Vec3 v, float c)
{
	return Vec3(v.x * c, v.y * c, v.z * c);
}

static Vec3 operator +(Vec3 a, Vec3 b)
{
	return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static Vec3 operator -(Vec3 a, Vec3 b)
{
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static Vec3 operator -(Vec3 v)
{
	return Vec3(-v.x, -v.y, -v.z);
}

// Represents a 2D image. 'pixels' contains an array of bytes,
// in the form r g b r g b r g b... 
// 'write' marks the next pixel location to write into.
struct Image
{
	uint8_t* pixels;
	int write = 0;

	int width, height;

	Image(int width, int height) : width(width), height(height)
	{
		pixels = (uint8_t*)malloc(width * height * CHANNELS);
	}

	// Set a pixel at the given index.
	void Set(int index, uint8_t r, uint8_t g, uint8_t b)
	{
		pixels[index] = r;
		pixels[index + 1] = g;
		pixels[index + 2] = b;
	}

	// Set a pixel at the given x, y pixel location.
	void Set(int x, int y, uint8_t r, uint8_t g, uint8_t b)
	{	
		// To convert a 2D coordinate into a flattened index, 
		// the general formula is: y * width + x when data is
		// stored in row-major order. Width in our case is the
		// pixel width * the number of bytes per pixel (channels). 
		// Every 1 pixel on x is 3 elements in the pixels array, 
		// so we must also multiply x by the channel count.
		int index = y * (width * CHANNELS) + (x * CHANNELS);
		Set(index, r, g, b);
	}

	// Takes a color with values between 0 and 1 and sets it to the
	// image as a color with values between 0 and 255.
	void Set(int x, int y, Vec3 color)
	{
		Vec3 final = color * 255.0f;
		Set(x, y, (uint8_t)final.x, (uint8_t)final.y, (uint8_t)final.z);
	}

	// Write the next pixel that hasn't been written to yet.
	// This was used for the test images.
	void Write(uint8_t r, uint8_t g, uint8_t b)
	{
		Set(write, r, g, b);
		write += 3;
	}

	// Write the image out to disk as a PNG file.
	void Output(const char* name)
	{
		stbi_write_png(name, width, height, CHANNELS, pixels, width * BYTES_PER_PIXEL);
	}

	~Image()
	{
		free(pixels);
	}
};

static float InverseSqrt(float v)
{
    return 1.0f / sqrtf(v);
}

// Dot product between two vectors.
static float Dot(Vec3 a, Vec3 b)
{
    Vec3 tmp(a * b);
    return tmp.x + tmp.y + tmp.z;
}

// Cross product between two vectors.
static Vec3 Cross(Vec3 a, Vec3 b)
{
    return Vec3(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y);
}

// Normalizes vector 'v', producing a unit vector.
static Vec3 Normalize(Vec3 v)
{
    return v * InverseSqrt(Dot(v, v));
}

static float Square(float v)
{
	return v * v;
}

static float Length2(Vec3 v)
{
    return Dot(v, v);
}

static float Distance2(Vec3 a, Vec3 b)
{
    return Length2(b - a);
}

enum Reflectivity
{
	DIFFUSE,
	REFLECTIVE
};

struct Camera
{
	Vec3 pos;
	Vec3 look;
};

typedef Vec3 Color;

struct Material
{
	Color color;
	int reflective;
};

struct Ray
{
	Vec3 pos;
	Vec3 dir;
};

struct RayHit
{
	float t;
	Vec3 normal;
	Vec3 point;
	Material mat;
};

struct Sphere
{
	Vec3 center;
	float radius;
	Material mat;
};

struct Triangle
{
	Vec3 a, b, c;
	Material mat;
};

struct Scene
{
	Camera cam = { Vec3(0.0f), Vec3(0.0f, 0.0f, -1.0f) };
	Vec3 light = Vec3(3.0f, 5.0f, -15.0f);

	Material refl = { Vec3(0.0f, 0.0f, 0.0f), 1 };
	Material blue = { Vec3(0.0f, 0.0f, 1.0f), 0 };
	Material red = { Vec3(1.0f, 0.0f, 0.0f), 0 };
	Material white = { Vec3(1.0f, 1.0f, 1.0f), 0 };

	Sphere spheres[3] = 
	{ 
		{ Vec3(0, 0, -16), 2, refl },
		{ Vec3(3, -1, -14), 1, refl },
		{ Vec3(-3, -1, -14), 1, red }
	};

	Triangle triangles[5] = 
	{
		{ Vec3(-8, -2, -20), Vec3(8, -2, -20), Vec3(8, 10, -20), blue },
		{ Vec3(-8, -2, -20), Vec3(8, 10, -20), Vec3(-8, 10, -20), blue },
		{ Vec3(-8, -2, -20), Vec3(8, -2, -10), Vec3(8, -2, -20), white },
		{ Vec3(-8, -2, -20), Vec3(-8, -2, -10),Vec3(8, -2, -10), white },
		{ Vec3(8, -2, -20),  Vec3(8, -2, -10), Vec3(8, 10, -20), red }
	};
};

static Vec3 PointOnRay(Ray ray, float t)
{
	return ray.pos + ray.dir * t;
}

// Given a pixel location on the image, generates a ray that faces
// the center of the corresponding pixel in world space.
static Ray GetRay(Scene* scene, int pixelX, int pixelY)
{	
	Camera* cam = &scene->cam;

	// Begin at -1. Add the number of pixels in world space to get to our location.
	// For y, subtract from (1 - world pixel size) in order to flip the result vertically.
	float targetX = -1.0f + pixelX * UNIT_PIXEL;
	float targetY = (1.0f - UNIT_PIXEL) - pixelY * UNIT_PIXEL;

	// Add half the world pixel size in order to end up in the center of the pixel.
	Vec3 target = Vec3(targetX + HALF_UNIT_PIXEL, targetY + HALF_UNIT_PIXEL, -2.0f);

	Vec3 dir = Normalize(target - cam->pos);

	return { cam->pos, dir };
}

// Functions for intersecting a ray with an object.
static RayHit RaySphereIntersect(Sphere sphere, Ray ray, float tMin)
{
	RayHit hit = { -1.0f };

	Vec3 rayToCenter = ray.pos - sphere.center;

	// Compute the discriminate from the ray-sphere intersection equation.
	// I skip taking the dot product of d with itself because d is normalized. This
	// value will always be 1.
	float discrim = Square(Dot(ray.dir, rayToCenter)) - (Dot(rayToCenter, rayToCenter) - Square(sphere.radius));

	// If 'discrim' < 0, there is no hit, otherwise there is a hit.
	// If there is a hit, return the actual t value for one solution.
	if (discrim >= 0.0f)
	{
		float v = Dot(-ray.dir, rayToCenter);
		float sq = sqrtf(discrim);

		// Compute both solutions to the equation and use the smallest valid solution.
		float tA = v - sq, tB = v + sq;

		bool validA = tA >= 0.0f && tA < tMin;
		bool validB = tB >= 0.0f && tB < tMin;

		float t;

		if (validA && validB)
			t = fminf(tA, tB);
		else if (validA)
			t = tA;
		else t = tB;

		if (validA || validB)
		{
			hit.t = t;
			hit.point = PointOnRay(ray, hit.t);
			hit.normal = Normalize(hit.point - sphere.center);
			hit.mat = sphere.mat;
		}
	}

	return hit;
}

static RayHit RayTriangleIntersect(Triangle triangle, Ray ray, float tMin)
{
	RayHit hit = { -1.0f };

	Vec3 a = triangle.a, b = triangle.b, c = triangle.c;

	float A = a.x - b.x;
	float B = a.y - b.y;
	float C = a.z - b.z;
	float D = a.x - c.x;
	float E = a.y - c.y;
	float F = a.z - c.z;
	float G = ray.dir.x;
	float H = ray.dir.y;
	float I = ray.dir.z;
	float J = a.x - ray.pos.x;
	float K = a.y - ray.pos.y;
	float L = a.z - ray.pos.z;

	float M = A * (E * I - H * F) + B * (G * F - D * I) + C * (D * H - E * G);

	float t = -(F * (A * K - J * B) + E * (J * C - A * L) + D * (B * L - K * C)) / M;

	if (t >= 0.0f && t < tMin)
	{
		float gamma = (I * (A * K - J * B) + H * (J * C - A * L) + G * (B * L - K * C)) / M;

		if (gamma >= 0.0f && gamma <= 1.0f)
		{
			float beta = (J * (E * I - H * F) + K * (G * F - D * I) + L * (D * H - E * G)) / M;

			if (beta >= 0.0f && beta <= 1.0f - gamma)
			{
				hit.t = t;
				hit.point = PointOnRay(ray, hit.t);
				hit.normal = Normalize(Cross(b - a, c - a));
				hit.mat = triangle.mat;
			}
		}
	}

	return hit;
}

static bool Raycast(Scene* scene, Ray ray, RayHit* result)
{
	RayHit closest;
	float tMin = FLT_MAX;

	// Perform a ray intersection with every object, tracking the
	// closest sphere intersected.

	// Test against all spheres.
	for (int i = 0; i < ArrayCount(scene->spheres); ++i)
	{
		RayHit hit = RaySphereIntersect(scene->spheres[i], ray, tMin);

		if (hit.t >= 0.0f)
		{
			tMin = hit.t;
			closest = hit;
		}
	}

	// Test against all triangles.
	for (int i = 0; i < ArrayCount(scene->triangles); ++i)
	{
		RayHit hit = RayTriangleIntersect(scene->triangles[i], ray, tMin);

		if (hit.t >= 0.0f)
		{
			tMin = hit.t;
			closest = hit;
		}
	}

	// If tMin is still FLT_MAX, we didn't hit anything.
	if (tMin < FLT_MAX)
	{
		*result = closest;
		return true; 
	}

	return false;
}

static Vec3 Reflect(Vec3 dir, Vec3 normal)
{
	return dir + -2.0f * Dot(dir, normal) * normal;
}

int main(int argc, char** argv)
{
	Scene scene = {};
	Image image = Image(512, 512);

	// For every pixel in the image, generate a ray and test to see if it 
	// intersects the sphere.
	for (int y = 0; y < IMAGE_PIXEL_SIZE; ++y)
	{
		for (int x = 0; x < IMAGE_PIXEL_SIZE; ++x)
		{
			Ray ray = GetRay(&scene, x, y);

			int triesLeft = 10;
			bool success = false;

			RayHit hit;

			// As long as we haven't hit a diffuse object and we have tries remaining,
			// keep casting rays.
			while (!success && triesLeft > 0)
			{
				if (Raycast(&scene, ray, &hit))
				{
					// If the ray hit, how we respond to the hit depends on
					// the surface type of the object hit.
					switch (hit.mat.reflective)
					{
						case DIFFUSE:
						{
							Vec3 toLight = Normalize(scene.light - hit.point);

							Vec3 shadowRayP = hit.point + EPSILON * toLight;
							Ray shadowRay = { shadowRayP, toLight };

							float distToLight2 = Distance2(scene.light, shadowRay.pos);

							// Diffuse brightness value.
							float diffuse;

							RayHit shadowHit;

							// If our shadow ray hits an object and the distance to it is less than the distance to the light,
							// the light is blocked and we shade this pixel. Since I am only comparing distances, squared 
							// distance is faster to compute.
							if (Raycast(&scene, shadowRay, &shadowHit) && Distance2(shadowRay.pos, shadowHit.point) < distToLight2)
								diffuse = AMBIENT;
							else diffuse = fmaxf(Dot(hit.normal, toLight), AMBIENT);

							Vec3 color = hit.mat.color * diffuse;
							image.Set(x, y, color);
							success = true;
						} break;

						case REFLECTIVE:
						{
							// The ray now starts at the hit location and points in the
							// reflected direction. We offset the position slightly to 
							// prevent visual artifacts.
							Vec3 newDir = Reflect(ray.dir, hit.normal);
							Vec3 newStart = hit.point + EPSILON * newDir;
							ray = { newStart, newDir };
							--triesLeft;
						} break;
					}
				}
				else triesLeft = 0; // Exit early, we hit nothing.
			}

			// Failed to hit an object (or reflected too many times).
			// Draw black in this case.
			if (!success)
				image.Set(x, y, 0, 0, 0);
		}
	}

	image.Output("reference.png");
}
