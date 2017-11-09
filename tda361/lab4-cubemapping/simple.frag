#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
uniform vec3 material_color;
uniform float material_reflectivity;
uniform float material_metalness;
uniform float material_fresnel;
uniform float material_shininess;
uniform float material_emission;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color = vec3(1.0, 1.0, 1.0);
uniform float point_light_intensity_multiplier = 50.0;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpaceNormal; 
in vec3 viewSpacePosition; 

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse; 
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;


vec3 calculateDirectIllumiunation(vec3 wo, vec3 n)
{
	///////////////////////////////////////////////////////////////////////////
	// Task 1.2 - Calculate the radiance Li from the light, and the direction
	//            to the light. If the light is backfacing the triangle, 
	//            return vec3(0); 
	///////////////////////////////////////////////////////////////////////////
	vec3 wi = normalize(viewSpaceLightPosition - viewSpacePosition);
	float d = length(viewSpaceLightPosition - viewSpacePosition);
	vec3 Li = point_light_intensity_multiplier * point_light_color * (1 / pow(d, 2));
	float dNWi = dot(n, wi);
	if(dNWi <= 0.0f)
		return vec3(0);

	///////////////////////////////////////////////////////////////////////////
	// Task 1.3 - Calculate the diffuse term and return that as the result
	///////////////////////////////////////////////////////////////////////////
	vec3 diffuse_term = material_color * (1.0/PI) * dNWi * Li;
	//return diffuse_term;

	///////////////////////////////////////////////////////////////////////////
	// Task 2 - Calculate the Torrance Sparrow BRDF and return the light 
	//          reflected from that instead
	///////////////////////////////////////////////////////////////////////////
	vec3 wh = (normalize(wi + wo));
	float s = material_shininess;
	float dNWh = dot(n, wh);
	float dNWo = dot(n, wo);
	float dWoWh = dot(wo, wh);

	float F = material_fresnel + (1 - material_fresnel) * pow((1 - dNWi), 5);
	float D = (s+2) / 2 * PI * pow(dNWh, s);
	float G = min(1, min(2 * dNWh * dNWo/dWoWh, 2 * dNWh * dNWi / dWoWh));
	float brdf = G * D * F / (length(dNWo) * length(dNWi));

	///////////////////////////////////////////////////////////////////////////
	// Task 3 - Make your shader respect the parameters of our material model.
	///////////////////////////////////////////////////////////////////////////
	vec3 dielectric_term = brdf * dNWi * Li + (1-F) * diffuse_term;

	vec3 metal_term = brdf * material_color * dNWi * Li;

	vec3 microfacet_term = material_metalness * metal_term + (1-material_metalness) * dielectric_term;

	return material_reflectivity * microfacet_term + (1 - material_reflectivity) * diffuse_term;
}

vec3 calculateIndirectIllumination(vec3 wo, vec3 n)
{
	///////////////////////////////////////////////////////////////////////////
	// Task 5 - Lookup the irradiance from the irradiance map and calculate
	//          the diffuse reflection
	///////////////////////////////////////////////////////////////////////////
	vec4 nws = viewInverse * vec4(n,0);
	vec3 fragmentColor;

	float theta = acos(max(-1.0f, min(1.0f, nws.y)));
	float phi = atan(nws.z, nws.x);
	if (phi < 0.0f) phi = phi + 2.0f * PI;

	vec2 lookup = vec2(phi / (2.0 * PI), theta / PI);

	fragmentColor = environment_multiplier * texture(irradianceMap, lookup).xyz;
	vec3 diffuse_term = material_color * 1.0f / PI * fragmentColor;
	
	//return diffuse_term;
	
	///////////////////////////////////////////////////////////////////////////
	// Task 6 - Look up in the reflection map from the perfect specular 
	//          direction and calculate the dielectric and metal terms. 
	///////////////////////////////////////////////////////////////////////////
	vec3 wi = vec3(viewInverse * vec4(reflect(-wo, n),0));
	float roughness = sqrt(sqrt(2 / (material_shininess + 2)));
	float dNWi = dot(n,wi);

	vec3 Li = environment_multiplier * textureLod(reflectionMap, lookup, roughness * 7.0).xyz;

	float F = material_fresnel + (1 - material_fresnel) * pow((1 - dNWi), 5);

	//if material is not metal, the light not reflected should be refracted into material
	vec3 dielectric_term = F * Li + (1-F) * diffuse_term;
	//refracted light takes on the material color for metal
	vec3 metal_term = F * material_color * Li;
	//blend dielectric and metal
	vec3 microfacet_term = material_metalness * metal_term + (1-material_metalness) * dielectric_term;

	return material_reflectivity * microfacet_term + (1 - material_reflectivity) * diffuse_term;
}


void main()
{
	///////////////////////////////////////////////////////////////////////////
	// Task 1.1 - Fill in the outgoing direction, wo, and the normal, n. Both
	//            shall be normalized vectors in view-space. 
	///////////////////////////////////////////////////////////////////////////
	vec3 wo = normalize(-viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);

	vec3 direct_illumination_term = vec3(0.0);
	{ // Direct illumination
		direct_illumination_term = calculateDirectIllumiunation(wo, n);
	}

	vec3 indirect_illumination_term = vec3(0.0);
	{ // Indirect illumination
		indirect_illumination_term = calculateIndirectIllumination(wo, n);
	}

	///////////////////////////////////////////////////////////////////////////
	// Task 7 - Make glowy things glow!
	///////////////////////////////////////////////////////////////////////////
	vec3 emission_term = material_emission * material_color;

	fragmentColor.xyz =
		direct_illumination_term +
		indirect_illumination_term +
		emission_term;
}
