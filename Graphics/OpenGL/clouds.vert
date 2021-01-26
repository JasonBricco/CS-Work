#version 150 // GLSL 150 = OpenGL 3.2

in vec3 in_Position;
in vec2 in_TexCoord;

uniform mat4 ModelView;
uniform mat4 Projection;

out vec2 out_TexCoord;

void main() 
{
	gl_Position = Projection * ModelView * vec4(in_Position, 1.0);
	out_TexCoord = in_TexCoord;
}
