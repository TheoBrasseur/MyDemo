#version 300 es

uniform sampler2D sTexture;

in highp vec2 texCoords;

out mediump vec4 fragmentColor;

void main()
{
  fragmentColor = texture(sTexture, texCoords) * vec4(0.0, 0.0, 1.0, 1.0);	
}
