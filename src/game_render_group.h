struct render_group
{
	u32 MaxBufferSize;
	u32 BufferSize;
	u8 *BufferBase;
};

enum render_group_entry_type
{
	RenderGroupEntryType_render_entry_sprite,
	RenderGroupEntryType_render_entry_glyph,
	RenderGroupEntryType_render_entry_lines,
	RenderGroupEntryType_render_entry_rect,
	RenderGroupEntryType_render_entry_rect_outline,
	RenderGroupEntryType_render_entry_transformation,
	RenderGroupEntryType_render_entry_clear,
	RenderGroupEntryType_render_entry_blend,
};

struct render_group_entry_header
{
	render_group_entry_type Type;
};

struct render_entry_sprite
{
    basis2d Basis;
    v2 Center;
    spritemap_offset Offset;
    layer_id Layer;
    v4 TintColor;
};

struct render_entry_glyph
{
	u32 TextureHandle;
	sprite_vertex Corners[4];
};

enum render_entry_lines_mode
{
	RenderEntryLinesMode_Simple,
	RenderEntryLinesMode_Smooth,
};
struct render_entry_lines
{
	v4 Color;
	render_entry_lines_mode Mode;
	u32 VertexCount;
};

struct render_entry_rect
{
	v4 Color;
	rect_vertex Corners[4];
};

struct render_entry_rect_outline
{
	v4 Color;
	rect_vertex OuterCorners[4];
	rect_vertex InnerCorners[4];
};

struct render_entry_transformation
{
	viewport Viewport;
	mat4 Projection;
};

struct render_entry_clear
{
	v4 ClearColor;
};

struct render_entry_blend
{
	b32 BlendEnabled;
};