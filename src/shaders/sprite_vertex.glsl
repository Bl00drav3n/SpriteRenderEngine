#version 330

in vec4 VertexPosition;
in ivec3 VertexSpriteOffset;
in vec4 VertexTint;

out ivec3 SpriteOffset;
out vec4  SpriteTint;

void main()
{
    gl_Position = VertexPosition;
    SpriteOffset = VertexSpriteOffset;
    SpriteTint = VertexTint;
}