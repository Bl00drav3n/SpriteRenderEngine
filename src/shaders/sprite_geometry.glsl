#version 330
layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform mat4 ProjectionTransform;
uniform vec2 SpriteHalfDim;
uniform vec2 SpriteTexDim;

in ivec3 SpriteOffset[];
in vec4  SpriteTint[];

out vec3 TexCoord;
out vec4 Tint;

void main() {
    float TexCoordZ = SpriteOffset[0].z;
    vec2 Offset = SpriteOffset[0].xy * SpriteTexDim;
    vec4 Center = gl_in[0].gl_Position;

    Tint = SpriteTint[0];

    TexCoord = vec3(Offset, TexCoordZ);
    gl_Position = ProjectionTransform * (Center + vec4(-SpriteHalfDim.x, -SpriteHalfDim.y, 0, 0));
    EmitVertex();

    TexCoord = vec3(Offset + vec2(SpriteTexDim.x, 0), TexCoordZ);
    gl_Position = ProjectionTransform * (Center + vec4(SpriteHalfDim.x, -SpriteHalfDim.y, 0, 0));
    EmitVertex();

    TexCoord = vec3(Offset + vec2(0, SpriteTexDim.y), TexCoordZ);
    gl_Position = ProjectionTransform * (Center + vec4(-SpriteHalfDim.x, SpriteHalfDim.y, 0, 0));
    EmitVertex();

    TexCoord = vec3(Offset + SpriteTexDim, TexCoordZ);
    gl_Position = ProjectionTransform * (Center + vec4(SpriteHalfDim.x, SpriteHalfDim.y, 0, 0));
    EmitVertex();
}