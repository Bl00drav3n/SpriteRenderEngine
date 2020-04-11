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
		f32 HalfWidth = 0.5f * Width;
		u32 VertexCount;
		switch(Mode) {
			case RenderEntryLinesMode_Simple:
			{
				VertexCount = Count;
				u32 Size = sizeof(v2) * VertexCount;
				v2 *Positions = (v2*)PushRenderEntryData_(Group, Size);
				CopyMemory(Positions, Points, Size);
			} break;
			case RenderEntryLinesMode_Smooth:
			{
				VertexCount = 2 * Count;
				v2 *PositionBase = (v2*)PushRenderEntryData_(Group, sizeof(v2) * VertexCount);
				if(PositionBase) {
					v2 *Position = PositionBase;
					v2 Normal, LastDir, NextDir;
					LastDir = Normalize(Points[1] - Points[0]);
					Normal = Perp(LastDir);
					*Position++ = Points[0] + HalfWidth * Normal;
					*Position++ = Points[0] - HalfWidth * Normal;
					for(u32 i = 1; i < Count - 1; i++) {
						v2 P = Points[i];
						v2 Q = Points[i + 1];
						NextDir = Normalize(Q - P);
						v2 Tangent = Normalize(LastDir + NextDir);
						v2 Miter = Perp(Tangent);
						f32 MiterLength = HalfWidth / Dot(Miter, Normal);
						Miter *= MiterLength;
		
						*Position++ = P + Miter;
						*Position++ = P - Miter;

						LastDir = NextDir;
						Normal = Perp(LastDir);
					}
					*Position++ = Points[Count - 1] + HalfWidth * Normal;
					*Position++ = Points[Count - 1] - HalfWidth * Normal;
				}
			} break;

			InvalidDefaultCase;
		}

		Entry->Color = Color;
		Entry->Mode = Mode;
		Entry->VertexCount = VertexCount;
	}
}

internal inline void DrawSprite(sprite_vertex Corners[4], f32 TintColor[4])
{
	glColor4fv(TintColor);
	glBegin(GL_TRIANGLE_STRIP);
	for(u32 i = 0; i < 4; i++) {
		glTexCoord2fv(Corners[i].TexCoord.E);
		glVertex2fv(Corners[i].Position.E);
	}
	glEnd();
}

void APIENTRY GLDebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    // TODO: Route through debug system? (can we get stack frame info?)
    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB:
        InvalidCodepath;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        InvalidCodepath;
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        InvalidCodepath;
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        InvalidCodepath;
        break;

        InvalidDefaultCase;
    }
}

internal inline void SetDefaultGLState()
{
    glColor4f(1.f, 1.f, 1.f, 1.f);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

#ifdef GAME_DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(GLDebugOutputCallback, 0);
#endif
}

internal void DrawRenderGroup(render_state *Renderer, render_group *Group)
{
    TIMED_FUNCTION();

    SetDefaultGLState();

    u32 SpriteVertexCount = 0; 
    spritemap_vertex *SpriteVertices = 0;
    // TODO: Create some persistant state for buffers etc. and only allocate/initialize once
    // TODO: This is old crap omg
    GLuint VertexArray;
    glGenVertexArrays(1, &VertexArray);
    glBindVertexArray(VertexArray);

    GLuint VertexBuffer;
    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_NUM_SPRITE_VERTICES * sizeof(spritemap_vertex), 0, GL_STATIC_DRAW);
    SpriteVertices = (spritemap_vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    glBindVertexArray(0);

    // TODO: We always use the last PushTransform call
    // TODO: What the fuck is this todo saying???
    mat4 ProjectionTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	for(u32 Offset = 0;
		Offset < Group->BufferSize;
		)
	{
		render_group_entry_header *Header = (render_group_entry_header*)(Group->BufferBase + Offset);
#define BEGIN_CASE(type) case RenderGroupEntryType_##type: { type *Entry = (type*)(Header + 1);
#define END_CASE_AND_BREAK Offset += sizeof(render_group_entry_header) + sizeof(*Entry); } break;
		switch(Header->Type) {
			BEGIN_CASE(render_entry_sprite)
			{
                if (SpriteVertexCount < MAX_NUM_SPRITE_VERTICES) {
                    spritemap_vertex *Vertex = SpriteVertices + SpriteVertexCount++;
                    Vertex->Position = Entry->Center;
                    Vertex->Offset = Entry->Offset;
                    // TODO: Premultiply alpha here or in shader?
                    Vertex->Tint = PremultiplyAlpha(Entry->TintColor);
                }
			} END_CASE_AND_BREAK;

			BEGIN_CASE(render_entry_glyph)
			{
#if 1
                // TODO: God help me
				glBindTexture(GL_TEXTURE_2D, Entry->TextureHandle);
				glBegin(GL_TRIANGLE_STRIP);
				for(u32 i = 0; i < 4; i++) {
					glTexCoord2fv(Entry->Corners[i].TexCoord.E);
					glVertex2fv(Entry->Corners[i].Position.E);
				}
				glEnd();
#endif
			} END_CASE_AND_BREAK;

			BEGIN_CASE(render_entry_lines)
			{
#if 1
                // TODO: NOOOOOOOOOOOPE
				f32 *PositionBase = (f32*)(Entry + 1);
				Offset += sizeof(v2) * Entry->VertexCount;

                // TODO: We are doing a little dance here to make compatibility mode work, remove asap
                // TODO: Holy shit yes please
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glBindVertexArray(0);
				glDisable(GL_TEXTURE_2D);
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2, GL_FLOAT, 0, PositionBase);
				glColor4fv(Entry->Color.E);
				switch(Entry->Mode) {
					case RenderEntryLinesMode_Simple:
					{
						glDrawArrays(GL_LINE_STRIP, 0, Entry->VertexCount);
					} break;
					case RenderEntryLinesMode_Smooth:
					{
						glDrawArrays(GL_TRIANGLE_STRIP, 0, Entry->VertexCount);
					} break;

					InvalidDefaultCase;
				}
				glDisableClientState(GL_VERTEX_ARRAY);
				glEnable(GL_TEXTURE_2D);
                glBindVertexArray(VertexArray);
                SpriteVertices = (spritemap_vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
#endif
			} END_CASE_AND_BREAK;
			
			BEGIN_CASE(render_entry_rect)
			{
#if 1
                // TODO: NOPE
				glDisable(GL_TEXTURE_2D);
				glColor4fv(Entry->Color.E);
				glBegin(GL_TRIANGLE_STRIP);
				for(u32 i = 0; i < 4; i++) {
					glVertex2fv(Entry->Corners[i].Position.E);
				}
				glEnd();
				SetDefaultGLState();
#endif
			} END_CASE_AND_BREAK;
			
			BEGIN_CASE(render_entry_rect_outline)
			{
#if 1
                // TODO: NOPE
				glDisable(GL_TEXTURE_2D);
				glColor4fv(Entry->Color.E);
				glBegin(GL_TRIANGLE_STRIP);
				glVertex2fv(Entry->OuterCorners[0].Position.E);
				glVertex2fv(Entry->InnerCorners[0].Position.E);
				glVertex2fv(Entry->OuterCorners[1].Position.E);
				glVertex2fv(Entry->InnerCorners[1].Position.E);
				glVertex2fv(Entry->OuterCorners[2].Position.E);
				glVertex2fv(Entry->InnerCorners[2].Position.E);
				glVertex2fv(Entry->OuterCorners[3].Position.E);
				glVertex2fv(Entry->InnerCorners[3].Position.E);
				glVertex2fv(Entry->OuterCorners[0].Position.E);
				glVertex2fv(Entry->InnerCorners[0].Position.E);
				glEnd();
				SetDefaultGLState();
#endif
			} END_CASE_AND_BREAK;

			BEGIN_CASE(render_entry_clear)
			{
				glClearColor(Entry->ClearColor.r, Entry->ClearColor.g, Entry->ClearColor.b, Entry->ClearColor.a);
				glClear(GL_COLOR_BUFFER_BIT);
			} END_CASE_AND_BREAK;
			
			BEGIN_CASE(render_entry_transformation)
			{
                // TODO: Load uniforms
                // TODO: Facepalm
#if 1
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glViewport(Entry->Viewport.X, Entry->Viewport.Y, Entry->Viewport.Width, Entry->Viewport.Height);
				glLoadMatrixf(Entry->Projection.ptr);
#endif

                ProjectionTransform = Entry->Projection;
			} END_CASE_AND_BREAK;

			BEGIN_CASE(render_entry_blend)
			{
				// NOTE: We are always using premultiplied alpha
				// DstColor = SrcColor + DstColor * (1 - SrcAlpha)
				if(Entry->BlendEnabled) {
					glEnable(GL_BLEND);
				}
				else {
					glDisable(GL_BLEND);
				}
			} END_CASE_AND_BREAK;

			InvalidDefaultCase;
		}
	}
#undef BEGIN_CASE
#undef END_CASE_AND_BREAK

    glBindVertexArray(VertexArray);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glVertexAttribPointer(VertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, sizeof(spritemap_vertex), (void*)offsetof(spritemap_vertex, Position));
    glVertexAttribPointer(VertexAttrib_SpriteOffset, 3, GL_INT, GL_FALSE, sizeof(spritemap_vertex), (void*)offsetof(spritemap_vertex, Offset));
    glVertexAttribPointer(VertexAttrib_Tint, 4, GL_FLOAT, GL_FALSE, sizeof(spritemap_vertex), (void*)offsetof(spritemap_vertex, Tint));

    for (u32 i = 0; i < ArrayCount(GlobalVertexAttribs); i++) {
        vertex_attrib *Attrib = GlobalVertexAttribs + i;
        glEnableVertexAttribArray(Attrib->Id);
    }

    sprite_program *SpriteProg = &Renderer->SpriteProgram;
    glUseProgram(SpriteProg->Id);

    u32 TexUnit = 0;
    spritemap_array *Spritemaps = &Renderer->Spritemaps;
    glActiveTexture(GL_TEXTURE0 + TexUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Spritemaps->TexId);

    v2 SpriteHalfDim = V2(2.5f, 2.5f);
    v2 SpriteTexDim = V2(1.f / (f32)SPRITEMAP_DIM_X, 1.f / (f32)SPRITEMAP_DIM_Y);
    glUniform1i(SpriteProg->SpritemapsLoc, TexUnit);
    glUniformMatrix4fv(SpriteProg->ProjectionTransformLoc, 1, GL_FALSE, ProjectionTransform.ptr);
    glUniform2fv(SpriteProg->SpriteHalfDimLoc, 1, SpriteHalfDim.E);
    glUniform2fv(SpriteProg->SpriteTexDimLoc, 1, SpriteTexDim.E);

    // TODO: Is this even necessary?
    GLuint OutputBuffer = GL_BACK_LEFT;
    glDrawBuffers(1, &OutputBuffer);

    // TODO: Remove query!!!
    GLuint Query;
    glGenQueries(1, &Query);
    glBeginQuery(GL_PRIMITIVES_GENERATED, Query);
    glDrawArrays(GL_POINTS, 0, SpriteVertexCount);
    glEndQuery(GL_PRIMITIVES_GENERATED);

    GLuint NumPrimitivesGenerated;
    glGetQueryObjectuiv(Query, GL_QUERY_RESULT, &NumPrimitivesGenerated);
    glDeleteQueries(1, &Query);

    // TODO: Keep vertex specification around (vertex array)
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &VertexBuffer);
    glDeleteVertexArrays(1, &VertexArray);
}
