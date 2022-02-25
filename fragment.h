static const char* fscode = R"(
#version 330 core

in vec3 vertexPosition_worldspace;
in vec3 eyeDirection_cameraspace;

layout(location = 0) out vec3 color;

void main() {
	vec3 ambientColour = vec3(0.3, 0.3, 0.3);
	vec3 diffuseColour = vec3(0.6, 0.6, 0.6);
	vec3 specularColour = vec3(0.1, 0.1, 0.1);
	vec3 light = normalize(vec3(1.0, -0.5, 0.8)); // y,-x,z in STL space. Camera is 1,-1,1

	vec3 normal = normalize(cross(dFdx(vertexPosition_worldspace), dFdy(vertexPosition_worldspace)));
	float cosTheta = clamp(dot(normal, light), 0.0, 1.0);

	vec3 E = normalize(eyeDirection_cameraspace);
	vec3 R = reflect(-light, normal);
	float cosAlpha = clamp(dot(E, R), 0, 1);

	color = vec3(ambientColour + diffuseColour * cosTheta + specularColour * pow(cosAlpha, 2));
}

)";