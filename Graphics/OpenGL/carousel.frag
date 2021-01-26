#version 150 // Specify which version of GLSL we are using.

out vec4 fragColor;
in vec3 out_Normal;

// Interpolated vertex position in camera coordinates.
in vec4 out_VertexPos;

// Light position in camera coordinates.
in vec4 out_LightP;

void main() 
{
	vec3 fragToCam = normalize(-out_VertexPos.xyz);
	vec3 fragToLight = normalize(out_LightP.xyz - out_VertexPos.xyz);

	vec3 halfDir = normalize(fragToCam + fragToLight);

	float spec = pow(max(dot(halfDir, out_Normal), 0.0), 10.0);

	// Diffuse shading equation, where light passes through to the opposing side.
	float diffuse = clamp(dot(fragToLight, out_Normal), 0.0, 1.0f);
	diffuse = diffuse / 2.0 + 0.5;

	fragColor.xyz = (spec + diffuse) * vec3(0.5, 0.5, 0.5);
	fragColor.a = 1;
}
