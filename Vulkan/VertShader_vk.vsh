#version	450 

#define POSITION_ARRAY	0

layout (location = POSITION_ARRAY) in highp vec4	inVertex;

layout (set = 0, binding = 0)uniform PerMesh
{
	highp mat4  MVPMatrix;		// model view projection transformation
};

void main()
{
	// Transform position
	gl_Position = MVPMatrix * inVertex;
}
