#version 330

in vec2 VertexPosition;
in ivec3 VertexSpriteOffset;
in vec4 VertexTint;

out ivec3 SpriteOffset;
out vec4  SpriteTint;

void main()
{
    gl_Position = vec4(VertexPosition, 0, 1);
    SpriteOffset = VertexSpriteOffset;
    SpriteTint = VertexTint;
}