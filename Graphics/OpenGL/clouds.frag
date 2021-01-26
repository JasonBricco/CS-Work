#version 150 // GLSL 150 = OpenGL 3.2

out vec4 fragColor;
in vec2 out_TexCoord;

uniform sampler2D tex;

void main() 
{
	vec4 sample = texture(tex, out_TexCoord);
	fragColor = vec4(1.0, 1.0, 1.0, sample.r);
}
