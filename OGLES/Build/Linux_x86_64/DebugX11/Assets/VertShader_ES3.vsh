#version 300 es

layout (location=0) in highp vec4 inPositions;

//out highp vec2 texCoords;b

void main()
{
	gl_Position =  inPositions;
}
