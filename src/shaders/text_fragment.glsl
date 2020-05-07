#version 330

uniform vec4 GlyphColor;
uniform sampler2D GlyphAtlasTex;

in vec2 TexCoord;

out vec4 FinalColor;
void main()
{
	//FinalColor = vec4(1.0, 1.0, 1.0, 1.0);
	//FinalColor = vec4(TexCoord, 0, 1);
    FinalColor = GlyphColor * texture(GlyphAtlasTex, TexCoord);
}