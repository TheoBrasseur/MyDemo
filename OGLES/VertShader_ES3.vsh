#version 300 es

layout (location=0) in highp vec4 inPositions;
layout (location=1) in highp vec2 inTexCoords;

out highp vec2 texCoords;

void main()
{
	gl_Position =  inPositions;
  texCoords = inTexCoords;
}
