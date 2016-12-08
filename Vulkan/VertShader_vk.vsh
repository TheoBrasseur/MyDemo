#version	450 

layout (location = 0) in highp vec4	inPositions;

layout (set = 0, binding = 0) uniform PerMesh
{
	highp mat4  MVPMatrix;		// model view projection transformation
};

void main()
{
	// Transform position
	gl_Position = MVPMatrix * inPositions;
}
