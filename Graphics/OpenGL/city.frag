#version 150 // GLSL 150 = OpenGL 3.2

out vec4 fragColor;
in vec3 color;
in float vertZ;

void main() 
{
	vec3 fogColor = vec3(0.12, 0.12, 0.12);

	float fogStart = 60.0;
	float fogEnd = 70.0;
	
	float fogFactor = clamp((fogEnd - vertZ) / (fogEnd - fogStart), 0.0, 1.0);
	fragColor.rgb = mix(fogColor, color.rgb, fogFactor);
}
