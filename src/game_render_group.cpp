internal render_group AllocateRenderGroup(mem_arena *Memory, u32 MaxSize)
{
	render_group Result = {};

	u8 *BufferBase = push_array(Memory, u8, MaxSize);
	if(BufferBase) {
		Result.BufferBase = BufferBase;
		Result.BufferSize = 0;
		Result.MaxBufferSize = MaxSize;
	}

	return Result;
}

#define PushRenderEntry(Group, type) (type*)PushRenderEntry_(Group, sizeof(type), RenderGroupEntryType_##type)
internal inline void * PushRenderEntry_(render_group *Group, u32 Size, render_group_entry_type Type)
{
	void *Result = 0;
	if(Group->BufferSize + Size + sizeof(render_group_entry_header) < Group->MaxBufferSize) {
		render_group_entry_header *Header = (render_group_entry_header*)(Group->BufferBase + Group->BufferSize);
		Header->Type = Type;
		Result = Header + 1;
		Group->BufferSize += Size + sizeof(render_group_entry_header);
	}

	return Result;
}

#define PushRenderEntryData(Group, type) (type*)PushRenderEntryData_(Group, sizeof(type))
internal void * PushRenderEntryData_(render_group *Group, u32 Size)
{
	void *Result = 0;
	if(Group->BufferSize + Size < Group->MaxBufferSize) {
		Result = Group->BufferBase + Group->BufferSize;
		Group->BufferSize += Size;
	}

	return Result;
}

internal void PushSprite(render_group *Group, layer_id Layer, v2 Pos, sprite *Sprite, basis2d Basis = CanonicalBasis(), v4 TintColor = V4(1.f, 1.f, 1.f, 1.f))
{
	render_entry_sprite *Entry = PushRenderEntry(Group, render_entry_sprite);
	if(Entry) {
        Entry->Basis = Basis;
		Entry->Layer = Layer;
		Entry->TintColor = TintColor;
        Entry->Center = Pos;
        Entry->Offset.X = Sprite->SpritePosX * SPRITE_WIDTH;
        Entry->Offset.Y = Sprite->SpritePosY * SPRITE_HEIGHT;
        Entry->Offset.Z = Sprite->SpritemapIndex;
	}
}

internal void PushRectangle(render_group *Group, rect2 R, v4 Color)
{
	render_entry_rect *Entry = PushRenderEntry(Group, render_entry_rect);
	if(Entry) {
		v2 MinCorner = GetMinCorner(R);
		v2 MaxCorner = GetMaxCorner(R);
		
		Entry->Color = Color;
		Entry->Corners[0].Position = V2(MinCorner.x, MinCorner.y);
		Entry->Corners[1].Position = V2(MaxCorner.x, MinCorner.y);
		Entry->Corners[2].Position = V2(MinCorner.x, MaxCorner.y);
		Entry->Corners[3].Position = V2(MaxCorner.x, MaxCorner.y);
	}
}

internal void PushRectangleOutline(render_group *Group, rect2 R, f32 Thickness, v4 Color)
{
	render_entry_rect_outline *Entry = PushRenderEntry(Group, render_entry_rect_outline);
	if(Entry) {
		v2 HalfBorder = V2(0.5f * Thickness, 0.5f * Thickness);
		v2 MinCorner = GetMinCorner(R);
		v2 MaxCorner = GetMaxCorner(R);
		v2 InnerMinCorner = MinCorner + HalfBorder;
		v2 OuterMinCorner = MinCorner - HalfBorder;
		v2 InnerMaxCorner = MaxCorner - HalfBorder;
		v2 OuterMaxCorner = MaxCorner + HalfBorder;
		
		Entry->Color = Color;
		Entry->OuterCorners[0].Position = V2(OuterMinCorner.x, OuterMinCorner.y);
		Entry->OuterCorners[1].Position = V2(OuterMaxCorner.x, OuterMinCorner.y);
		Entry->OuterCorners[2].Position = V2(OuterMaxCorner.x, OuterMaxCorner.y);
		Entry->OuterCorners[3].Position = V2(OuterMinCorner.x, OuterMaxCorner.y);
		Entry->InnerCorners[0].Position = V2(InnerMinCorner.x, InnerMinCorner.y);
		Entry->InnerCorners[1].Position = V2(InnerMaxCorner.x, InnerMinCorner.y);
		Entry->InnerCorners[2].Position = V2(InnerMaxCorner.x, InnerMaxCorner.y);
		Entry->InnerCorners[3].Position = V2(InnerMinCorner.x, InnerMaxCorner.y);
	}
}

internal inline void PushTransformation(render_group *Group, viewport Viewport, mat4 Projection)
{
	render_entry_transformation *Entry = PushRenderEntry(Group, render_entry_transformation);
	if(Entry) {
		Entry->Viewport = Viewport;
		Entry->Projection = Projection;
	}
}

internal inline void PushClear(render_group *Group, v4 ClearColor)
{
	render_entry_clear *Entry = PushRenderEntry(Group, render_entry_clear);
	if(Entry) {
		Entry->ClearColor = ClearColor;
	}
}

internal inline void PushBlend(render_group *Group, b32 Enabled)
{
	render_entry_blend *Entry = PushRenderEntry(Group, render_entry_blend);
	if(Entry) {
		Entry->BlendEnabled = Enabled;
	}
}

internal void PushGlyph(render_group *Group, font *Font, v2 P, u8 Codepoint)
{
	render_entry_glyph *Entry = PushRenderEntry(Group, render_entry_glyph);
	if(Entry) {
		glyph *Glyph = Font->Data->Glyphs + Codepoint;
		v2 Origin = P + V2(Glyph->LeftSideBearing, -(f32)Glyph->OffsetY);
		texture *Tex = Font->GlyphTextures[Codepoint];
		v2 LowerLeftOffset = V2(0.5f / (f32)Tex->Width, 0.5f / (f32)Tex->Height);
		v2 UpperRightOffset = V2(1.f, 1.f) - LowerLeftOffset;

		Entry->TextureHandle = Tex->Handle;
		Entry->Corners[0].Position = V2(Origin.x, Origin.y);
		Entry->Corners[1].Position = V2(Origin.x + Glyph->Width, Origin.y);
		Entry->Corners[2].Position = V2(Origin.x, Origin.y - Glyph->Height);
		Entry->Corners[3].Position = V2(Origin.x + Glyph->Width, Origin.y - Glyph->Height);
		Entry->Corners[0].TexCoord = V2(LowerLeftOffset.x, LowerLeftOffset.y);
		Entry->Corners[1].TexCoord = V2(UpperRightOffset.x, LowerLeftOffset.y);
		Entry->Corners[2].TexCoord = V2(LowerLeftOffset.x, UpperRightOffset.y);
		Entry->Corners[3].TexCoord = V2(UpperRightOffset.x, UpperRightOffset.y);
	}
}

internal void PushLines(render_group *Group, v2 *Points, u32 Count, f32 Width, render_entry_lines_mode Mode, v4 Color = V4(1.f, 1.f, 1.f, 1.f))
{
	if(Count == 0)
		return;

	render_entry_lines *Entry = PushRenderEntry(Group, render_entry_lines);
	if(Entry) {
		Entry->VertexCount = Count;
		Entry->Width = Width;
		Entry->Color = Color;
		Entry->Mode = Mode;

		u32 Size = sizeof(*Points) * Count;
		v2 *Positions = (v2*)PushRenderEntryData_(Group, Size);
		CopyMemory(Positions, Points, Size);
	}
}