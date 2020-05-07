#version 330

in vec2 VertexPosition;
in vec2 VertexGlyphDim;
in int VertexIndex;

out int GlyphIndex;

void main()
{
    gl_Position = vec4(VertexPosition, 0, 1);
	GlyphIndex = VertexIndex;
}