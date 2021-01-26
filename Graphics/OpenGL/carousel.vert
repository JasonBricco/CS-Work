#version 150 // Specify which version of GLSL we are using.

in vec3 in_Position;
in vec3 in_Normal;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat4 GeomTransform;

out vec3 out_Normal;
out vec4 out_VertexPos;
out vec4 out_LightP;

void main()
{
	vec4 pos = vec4(in_Position, 1.0);
	mat3 NormalMat = mat3(transpose(inverse(ModelView * GeomTransform)));
	out_Normal = normalize(NormalMat * in_Normal);

	vec4 lightP = vec4(13.4, 250.0, -3.7, 1.0);
	out_LightP = ModelView * GeomTransform * lightP;

	// Compute the vertex position in camera space.
	out_VertexPos = ModelView * GeomTransform * pos;

	gl_Position = Projection * ModelView * GeomTransform * pos;
}
