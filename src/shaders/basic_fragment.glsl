#version 330

uniform sampler2D Tex;

in vec2 TexCoord;
in vec4 Color;

out vec4 FinalColor;
void main()
{
	FinalColor = Color;// * texture(Tex, TexCoord);
}