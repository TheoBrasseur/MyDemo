#version 300 es

layout (location=0) in highp vec4 inPositions;
layout (location=1) in highp vec4 inTexCoords;

out highp vec2 fragCoords;

uniform mat4 mvp;

void main()
{
	gl_Position =  mvp * inPositions;
  fragCoords = gl_Position;
}
