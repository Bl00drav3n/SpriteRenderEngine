#version 330
layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform mat4 ProjectionTransform;
uniform samplerBuffer GlyphDataBuffer;
uniform ivec2 GlyphTileSize;
uniform ivec2 GlyphAtlasSize;

in int GlyphIndex[];

out vec2 TexCoord;

void main() {
	ivec2 Index = ivec2(GlyphIndex[0] % GlyphAtlasSize.x, GlyphIndex[0] / GlyphAtlasSize.x);
	vec2 TileTexelSize = 1 / vec2(GlyphAtlasSize);
	vec2 TexelSize = 1 / vec2(GlyphAtlasSize * GlyphTileSize);
	vec2 TexCoordOffset = vec2(Index) * TileTexelSize;
	vec4 GlyphData = texelFetch(GlyphDataBuffer, GlyphIndex[0]);
	vec2 GlyphOffset = GlyphData.xy;
	vec2 GlyphSize = GlyphData.zw;
	vec2 GlyphTexelSize = TileTexelSize;
	
	vec4 Corners[4];
    vec2 LowerLeft = gl_in[0].gl_Position.xy - vec2(0, GlyphTileSize.y + GlyphOffset.y);
	vec2 UpperRight = LowerLeft + GlyphTileSize;

	vec2 TexCoords[4];
	vec2 TexLowerLeft = TexCoordOffset;
	vec2 TexUpperRight = TexLowerLeft + GlyphTexelSize;

	Corners[0] = vec4(LowerLeft.x, LowerLeft.y, 0, 1);
	TexCoords[0] = vec2(TexLowerLeft.x, TexLowerLeft.y);

	Corners[1] = vec4(UpperRight.x, LowerLeft.y, 0, 1);
	TexCoords[1] = vec2(TexUpperRight.x, TexLowerLeft.y);

	Corners[2] = vec4(LowerLeft.x, UpperRight.y, 0, 1);
	TexCoords[2] = vec2(TexLowerLeft.x, TexUpperRight.y);

	Corners[3] = vec4(UpperRight.x, UpperRight.y, 0, 1);
	TexCoords[3] = vec2(TexUpperRight.x, TexUpperRight.y);

	for(int i = 0; i < 4; i++) {
		gl_Position = ProjectionTransform * Corners[i];
		TexCoord = TexCoords[i];
		EmitVertex();
	}
}
