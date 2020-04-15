#version 330

uniform mat4 ProjectionTransform;

in vec2 VertexPosition;
in vec2 VertexTexCoord;
in vec4 VertexTint;

out vec2 TexCoord;
out vec4 Color;

void main()
{
    gl_Position = ProjectionTransform * vec4(VertexPosition, 0, 1);
	TexCoord = VertexTexCoord;
    Color = VertexTint;
}