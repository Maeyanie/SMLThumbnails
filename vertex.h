static const char* vscode = R"(
#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;

out vec3 vertexPosition_worldspace;
out vec3 eyeDirection_cameraspace;

uniform mat4 MVP;
uniform mat4 M;
uniform mat4 V;

void main() {
	gl_Position = MVP * vec4(vertexPosition_modelspace, 1);

	// Position of the vertex, in worldspace : M * position
	vertexPosition_worldspace = (M * vec4(vertexPosition_modelspace,1)).xyz;

	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vec3 vertexPosition_cameraspace = ( V * M * vec4(vertexPosition_modelspace,1)).xyz;
	eyeDirection_cameraspace = vec3(0,0,0) - vertexPosition_cameraspace;
}

)";