#version 150 // GLSL 150 = OpenGL 3.2

out vec4 fragColor;
in vec2 out_TexCoord;

in vec4 out_VertexPos;
in vec3 out_Normal;

uniform sampler2D color;

void main() 
{
	vec3 lightPos = vec3(0.0, 0.0, 0.0);
	vec3 lightDir = normalize(lightPos - out_VertexPos.xyz);

	float diffuse = dot(lightDir, out_Normal.xyz);
	diffuse = clamp(diffuse, 0.0, 1.0);
	diffuse = diffuse / 2.0 + 0.5;

	fragColor = texture(color, out_TexCoord) * diffuse;
	fragColor.a = 1.0;
}
