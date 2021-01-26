#version 150 // GLSL 150 = OpenGL 3.2

in vec3 in_Position;
in vec2 in_TexCoord;

uniform mat4 ModelView;
uniform mat4 Projection;

out vec4 out_VertexPos;
out vec3 out_Normal;

out vec2 out_TexCoord;
uniform sampler2D heightmap;

void main() 
{
	float scaleFactor = 80.0;
	
	// How far to move a single pixel along the texture.
	float texelDistX = 1.0 / 2048.0;
	float texelDistY = 1.0 / 1024.0;

	// Distance to move a quad on the terrain.
	float quadDistX = 0.5;
	float quadDistZ = 0.5;

	// Sample three points on the heightmap texture to form a triangle.
	vec4 sampleC = texture(heightmap, in_TexCoord);
	vec4 sampleL = texture(heightmap, vec2(in_TexCoord.x - texelDistX, in_TexCoord.y));
	vec4 sampleU = texture(heightmap, vec2(in_TexCoord.x, in_TexCoord.y + texelDistY));

	// Get the vertex position at each sample location.
	vec3 posC = vec3(in_Position.x, in_Position.y + (sampleC.r * scaleFactor), in_Position.z);
	vec3 posR = vec3(in_Position.x + quadDistX, in_Position.y + (sampleL.r * scaleFactor), in_Position.z);
	vec3 posU = vec3(in_Position.x, in_Position.y + (sampleU.r * scaleFactor), in_Position.z + quadDistZ);

	gl_Position = Projection * ModelView * vec4(posC, 1.0);

	out_TexCoord = in_TexCoord;
	out_VertexPos = ModelView * vec4(posC, 1.0);

	// Compute the normal and convert it into camera space.
	mat3 NormalMat = mat3(transpose(inverse(ModelView)));
	out_Normal = normalize(cross(posU - posC, posR - posC));
	out_Normal = normalize(NormalMat * out_Normal);
}
