#version 150 // GLSL 150 = OpenGL 3.2

out vec4 fragColor;

in vec2 out_TexCoord; // Vertex texture coordinate
in vec3 out_Color;    // Vertex color
in vec3 out_Normal;   // Normal vector in camera coordinates
in vec3 out_Tangent;
in vec3 out_Bitangent;
in vec3 out_CamCoord; // Position of fragment in camera coordinates
in mat3 out_NormalMat;

uniform sampler2D tex; // Diffuse texture
uniform sampler2D tex_NORMALS; // Normal map.
uniform sampler2D tex_SPECULAR; // Specular map.

uniform int renderStyle;

/** Calculate diffuse shading. Normal and light direction do not need
 * to be normalized. */
float diffuseScalar(vec3 normal, vec3 lightDir, bool frontBackSame)
{
	/* Basic equation for diffuse shading */
	float diffuse = dot(lightDir, normal);

	/* The diffuse value will be negative if the normal is pointing in
	 * the opposite direction of the light. Set diffuse to 0 in this
	 * case. Alternatively, we could take the absolute value to light
	 * the front and back the same way. Either way, diffuse should now
	 * be a value from 0 to 1. */
	if (frontBackSame)
		diffuse = abs(diffuse);
	else diffuse = clamp(diffuse, 0, 1);

	/* Keep diffuse value in range from .5 to 1 to prevent any object
	 * from appearing too dark. Not technically part of diffuse
	 * shading---however, you may like the appearance of this. */
	diffuse = diffuse / 2.0 + 0.5;

	return diffuse;
}

float getSpecular(vec3 normal, vec3 lightDir, vec3 camDir)
{
	vec3 halfDir = normalize(camDir + lightDir);
	return pow(max(dot(halfDir, normal), 0.0), 10.0);
}

void main() 
{
	vec3 texNormal = texture(tex_NORMALS, out_TexCoord).xyz;
	texNormal = (pow(texNormal, vec3(1.0 / 2.2))) * 2.0 - 1.0;

	mat3 tbn;
	tbn[0] = out_Tangent;
	tbn[1] = out_Bitangent;
	tbn[2] = out_Normal;

	vec3 adjustedNormal = normalize(out_NormalMat * tbn * texNormal);

	/* Get position of light in camera coordinates. When we do
	 * headlight style rendering, the light will be at the position of
	 * the camera! */
	vec3 lightPos = vec3(0.0, 1000.0, 0.0);

	/* Calculate a vector pointing from our current position (in
	 * camera coordinates) to the light position. */
	vec3 lightDir = normalize(lightPos - out_CamCoord);

	/* Calculate diffuse shading */
	vec3 normal = normalize(out_NormalMat * out_Normal);

	float original_diffuse = diffuseScalar(normal, lightDir, false);
	float adjusted_diffuse = diffuseScalar(adjustedNormal, lightDir, false);

	if (renderStyle == 0) // Diffuse shading (no color).
	{
		fragColor = vec4(original_diffuse, original_diffuse, original_diffuse, 1);
	}
	else if (renderStyle == 1) // Diffuse + normalmap (no color).
	{
		fragColor = vec4(adjusted_diffuse, adjusted_diffuse, adjusted_diffuse, 1);
	}
	else if (renderStyle == 2) // Normals as color.
	{
		/* Normal coloring: Each component in the normals ranges from -1 to 1. Make them range from 0 to 1. */
		fragColor.xyz = (normal + 1.0) / 2.0;
		fragColor.a = 1;
	}
	else if (renderStyle == 3) // Normals as color (normalmap).
	{
		fragColor.xyz = (adjustedNormal + 1.0) / 2.0;
		fragColor.a = 1;
	}
	else if (renderStyle == 4) // Diffuse + color texture, original normal.
	{
		fragColor = texture(tex, out_TexCoord);
		fragColor.xyz *= original_diffuse;
	}
	else if (renderStyle == 5) // Diffuse + color texture, adjusted normal.
	{
		fragColor = texture(tex, out_TexCoord);
		fragColor.xyz *= adjusted_diffuse;
	}
	else if (renderStyle == 6) // Diffuse + color texture + specular, adjusted normal.
	{
		vec3 camDir = normalize(-out_CamCoord);

		// The amount of specular from the specular map.
		float how_much_specular = pow(texture(tex_SPECULAR, out_TexCoord).r, 1.0 / 2.2);

		// The real specular value, scaled by the specular map.
		float specular = getSpecular(adjustedNormal, lightDir, camDir) * how_much_specular;

		// Apply the specular/diffuse values to the model.
		fragColor = texture(tex, out_TexCoord);
		fragColor.rgb *= adjusted_diffuse;
		fragColor.rgb += specular;
	}
}
