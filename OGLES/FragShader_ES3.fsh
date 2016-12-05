#version 300 es

layout (location = 0) uniform sampler2D samplerColor;
layout (location = 1) uniform sampler2D samplerDepth;

uniform mat4 modelViewIT;
uniform mat4 prevModelViewProj;

in highp fragCoords;

out mediump vec4 fragmentColor;

void main()
{
  vec4 current = modelViewIT * fragCoords;
  vec4 previous = prevModelViewProj * current;

  depth = texture(samplerDepth, fragCoords).x;

	fragmentColor = texture(depth, fragCoords);	
}
