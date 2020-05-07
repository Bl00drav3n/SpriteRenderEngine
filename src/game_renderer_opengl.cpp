#include "game_opengl.h"

enum opengl_texture_unit_assignment {
	TEXTURE_UNIT_SPRITEMAP,
	TEXTURE_UNIT_GLYPH_ATLAS,
	TEXTURE_UNIT_GLYPH_DATA,
};

struct opengl_state {
	GLuint SpriteVertexArray;
	GLuint SpriteVertexBuffer;

	GLuint BasicVertexArray;
	GLuint BasicVertexBuffer;
};

enum vertex_attrib_id
{
    VertexAttrib_Position,
    VertexAttrib_TexCoord,
    VertexAttrib_SpriteOffset,
    VertexAttrib_Tint,
	VertexAttrib_Index,

    VertexAttrib_Count
};

struct vertex_attrib
{
    vertex_attrib_id Id;
    char *Name;
};

global_persist vertex_attrib GlobalVertexAttribs[] = {
    { VertexAttrib_Position, "VertexPosition" },
    { VertexAttrib_TexCoord, "VertexTexCoord" },
    { VertexAttrib_SpriteOffset, "VertexSpriteOffset" },
    { VertexAttrib_Tint, "VertexTint" },
	{ VertexAttrib_Index, "VertexIndex" },
};

void ReloadRenderBackend()
{
	// NOTE: Some tiny meta-preprocessing so we only need to add things once
#define GL_MACRO(name, ...) name = (_##name*)Platform->LoadProcAddressGL(#name)
#include "game_opengl.h"
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
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

#ifdef GAME_DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(GLDebugOutputCallback, 0);
#endif
}

internal texture * CreateTexture(render_state *State, u32 Width, u32 Height, u8 *Data)
{
	texture * Result = AllocateTexture(State);
	if(Result) 
	{
		glGenTextures(1, &Result->Handle);
		glBindTexture(GL_TEXTURE_2D, Result->Handle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		Result->Width = Width;
		Result->Height = Height;
		Result->HasBeenFreed = false;

		// TODO: Store more information about the texture? (size, bpp, type, ...)
		State->TextureMemoryUsed += 4 * Width * Height;
	}

	return Result;
}

internal void UpdateSpritemapSprite(spritemap_array *Spritemaps, u32 OffsetX, u32 OffsetY, u32 Index, sprite_pixel *Pixels) {
    glBindTexture(GL_TEXTURE_2D_ARRAY, Spritemaps->TexId);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, OffsetX, OffsetY, Index, SPRITE_WIDTH, SPRITE_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

internal GLuint CreateShader(char *src, GLenum Type)
{
	GLuint Shader = glCreateShader(Type);
	glShaderSource(Shader, 1, (const char**)&src, 0);
	glCompileShader(Shader);
	
#if 1
	GLint Status;
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &Status);
	if(!Status) {
		DebugConsolePushString("Shader compilation failed.", MSG_ERROR);
		GLsizei Length;
		char Buffer[1024];
		glGetShaderInfoLog(Shader, sizeof(Buffer), &Length, Buffer);
		if(Length) {
			DebugConsolePushString(Buffer);
		}
	}
#endif

	return Shader;
}

internal void BindGlobalVertexAttributes(GLuint Program) {
    Assert(ArrayCount(GlobalVertexAttribs) == VertexAttrib_Count);
    for (u32 i = 0; i < ArrayCount(GlobalVertexAttribs); i++) {
        vertex_attrib *Attrib = GlobalVertexAttribs + i;
        glBindAttribLocation(Program, Attrib->Id, Attrib->Name);
    }
}

internal GLuint CreateShaderProgram(char *Sources[3])
{
    GLuint Program = glCreateProgram();

    GLenum Types[3] = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER };
    for (u32 i = 0; i < 3; i++) {
        char *Src = Sources[i];
        GLenum Type = Types[i];
        if (Src) {
            GLuint Shader = CreateShader(Src, Type);
            glAttachShader(Program, Shader);
        }
    }

    BindGlobalVertexAttributes(Program);

    glBindFragDataLocation(Program, 0, "FinalColor");
    
	glLinkProgram(Program);

	GLint Status;
	glGetProgramiv(Program, GL_LINK_STATUS, &Status);
	if(!Status) {
		DebugConsolePushString("Could not create shader program.", MSG_ERROR);

		GLsizei Length;
		char Buffer[1024];
		glGetProgramInfoLog(Program, sizeof(Buffer), &Length, Buffer);
		if(Length) {
			DebugConsolePushString(Buffer);
		}
	}

    return Program;
}

internal GLuint LoadProgram(mem_arena *Memory, char *ShaderSourceNames[3]) {
	char *ShaderSources[3];
	temporary_memory TempMem = BeginTempMemory(Memory);
	LoadShaderSources(Memory, ShaderSourceNames, ShaderSources);
	u32 Result = CreateShaderProgram(ShaderSources);
	EndTempMemory(&TempMem);
	return Result;
}

internal basic_program CreateBasicProgram(mem_arena *Memory)
{    
	basic_program Result;
	char *ShaderSourceNames[3] = { "basic_vertex", 0, "basic_fragment" };
    Result.Id = LoadProgram(Memory, ShaderSourceNames);

    return Result;
}

internal line_program CreateLineProgram(mem_arena *Memory)
{    
	line_program Result;
	char *ShaderSourceNames[3] = { "line_vertex", "line_geometry", "line_fragment" };
    Result.Id = LoadProgram(Memory, ShaderSourceNames);

    return Result;
}

internal sprite_program CreateSpriteProgram(mem_arena *Memory)
{
	sprite_program Result;
	char *ShaderSourceNames[3] = { "sprite_vertex", "sprite_geometry", "sprite_fragment" };
    Result.Id = LoadProgram(Memory, ShaderSourceNames);

    return Result;
}

internal text_program CreateTextProgram(mem_arena *Memory)
{
	text_program Result;
	char *ShaderSourceNames[3] = { "text_vertex", "text_geometry", "text_fragment" };
    Result.Id = LoadProgram(Memory, ShaderSourceNames);

    return Result;
}

internal void RenderSprites(render_state *Renderer, spritemap_vertex *Vertices, u32 VertexCount, const mat4 &ProjectionTransform)
{   
	TIMED_FUNCTION();

	opengl_state *State = (opengl_state*)Renderer->Backend.State;
	GLuint VertexArray = State->SpriteVertexArray;
	GLuint VertexBuffer = State->SpriteVertexBuffer;
		
	glBindVertexArray(VertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, VertexCount * sizeof(*Vertices), Vertices);

	sprite_program *SpriteProg = &Renderer->SpriteProgram;
	glUseProgram(SpriteProg->Id);

	u32 TexUnit = TEXTURE_UNIT_SPRITEMAP;
	spritemap_array *Spritemaps = Renderer->Spritemaps;
	glActiveTexture(GL_TEXTURE0 + TexUnit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, Spritemaps->TexId);

	v2 SpriteHalfDim = V2(2.5f, 2.5f);
	v2 SpriteTexDim = V2(1.f / (f32)SPRITEMAP_DIM_X, 1.f / (f32)SPRITEMAP_DIM_Y);
		
	GLint ProjectionTransformLoc = glGetUniformLocation(SpriteProg->Id, "ProjectionTransform");
	GLint SpriteHalfDimLoc = glGetUniformLocation(SpriteProg->Id, "SpriteHalfDim");
	GLint SpriteTexDimLoc = glGetUniformLocation(SpriteProg->Id, "SpriteTexDim");
	GLint SpritemapsLoc = glGetUniformLocation(SpriteProg->Id, "Spritemaps");

	// TODO: ProjectionTransform can happen anywhere in the pipeline. We might need to track it.
	glUniform1i(SpritemapsLoc, TexUnit);
	glUniformMatrix4fv(ProjectionTransformLoc, 1, GL_FALSE, ProjectionTransform.ptr);
	glUniform2fv(SpriteHalfDimLoc, 1, SpriteHalfDim.E);
	glUniform2fv(SpriteTexDimLoc, 1, SpriteTexDim.E);

	// TODO: Is this even necessary? (probably not)
	GLuint OutputBuffer = GL_BACK_LEFT;
	glDrawBuffers(1, &OutputBuffer);

#if 0
	// TODO: Remove query!!!
	GLuint Query;
	glGenQueries(1, &Query);
	glBeginQuery(GL_PRIMITIVES_GENERATED, Query);
	glDrawArrays(GL_POINTS, 0, SpriteVertexCount);
	glEndQuery(GL_PRIMITIVES_GENERATED);

	GLuint NumPrimitivesGenerated;
	glGetQueryObjectuiv(Query, GL_QUERY_RESULT, &NumPrimitivesGenerated);
	glDeleteQueries(1, &Query);
#else
	glDrawArrays(GL_POINTS, 0, VertexCount);
#endif
	glUseProgram(0);
	glBindVertexArray(0);
}

internal void InitializeRenderBackend(render_state *Renderer)
{
	Assert(Renderer->Backend.State == 0);
	opengl_state *State = push_type(&Renderer->Memory, opengl_state);
	Renderer->Backend.State = State;	
	
	// NOTE: Basic
	glGenVertexArrays(1, &State->BasicVertexArray);
	glBindVertexArray(State->BasicVertexArray);
		
	glGenBuffers(1, &State->BasicVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, State->BasicVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_NUM_BASIC_VERTICES * sizeof(basic_vertex), 0, GL_STATIC_DRAW);

	glVertexAttribPointer(VertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, sizeof(basic_vertex), (void*)offsetof(basic_vertex, Position));
	glEnableVertexAttribArray(GlobalVertexAttribs[VertexAttrib_Position].Id);

	glVertexAttribPointer(VertexAttrib_TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(basic_vertex), (void*)offsetof(basic_vertex, TexCoord));
	glEnableVertexAttribArray(GlobalVertexAttribs[VertexAttrib_TexCoord].Id);

	glVertexAttribPointer(VertexAttrib_Tint, 4, GL_FLOAT, GL_FALSE, sizeof(basic_vertex), (void*)offsetof(basic_vertex, Tint));
	glEnableVertexAttribArray(GlobalVertexAttribs[VertexAttrib_Tint].Id);

	// NOTE: Sprites
	glGenVertexArrays(1, &State->SpriteVertexArray);
	glBindVertexArray(State->SpriteVertexArray);

	glGenBuffers(1, &State->SpriteVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, State->SpriteVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, MAX_NUM_SPRITE_VERTICES * sizeof(spritemap_vertex), 0, GL_STREAM_DRAW);
	
	glVertexAttribPointer(VertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, sizeof(spritemap_vertex), (void*)offsetof(spritemap_vertex, Position));
	glEnableVertexAttribArray(GlobalVertexAttribs[VertexAttrib_Position].Id);
	
	glVertexAttribPointer(VertexAttrib_SpriteOffset, 3, GL_INT, GL_FALSE, sizeof(spritemap_vertex), (void*)offsetof(spritemap_vertex, Offset));
	glEnableVertexAttribArray(GlobalVertexAttribs[VertexAttrib_SpriteOffset].Id);
	
	glVertexAttribPointer(VertexAttrib_Tint, 4, GL_FLOAT, GL_FALSE, sizeof(spritemap_vertex), (void*)offsetof(spritemap_vertex, Tint));
	glEnableVertexAttribArray(GlobalVertexAttribs[VertexAttrib_Tint].Id);

	// NOTE: Spritemaps
	{
		spritemap_array *Map = push_type(&Renderer->Memory, spritemap_array);
		GLuint TexId;
		glGenTextures(1, &TexId);
		if (TexId) {
			u32 TextureWidth = SPRITEMAP_DIM_X * SPRITE_WIDTH;
			u32 TextureHeight = SPRITEMAP_DIM_Y * SPRITE_HEIGHT;

			glBindTexture(GL_TEXTURE_2D_ARRAY, TexId);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, TextureWidth, TextureHeight, SPRITEMAP_COUNT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			Map->TexId = TexId;
			Map->TextureWidth = TextureWidth;
			Map->TextureHeight = TextureHeight;
			// TODO: Use a stub spritemap for release so we don't crash if we run out of resources (unlikely)?
			Renderer->Spritemaps = Map;
		}
	}

	// NOTE: Glyph map


	GLuint GlyphTBO, GlyphDataTexture;
	glGenBuffers(1, &GlyphTBO);
	glGenTextures(1, &GlyphDataTexture);

	v4 GlyphData[GLYPH_ATLAS_COUNT_Y * GLYPH_ATLAS_COUNT_X] = {};
	for(u32 i = 0; i < ArrayCount(Renderer->DebugFont.Data->Glyphs); i++) {
		glyph *Glyph = Renderer->DebugFont.Data->Glyphs + i;
		GlyphData[i] = V4((f32)Glyph->OffsetX, (f32)Glyph->OffsetY, (f32)Glyph->Width, (f32)Glyph->Height);
	}

	glBindBuffer(GL_TEXTURE_BUFFER, GlyphTBO);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(GlyphData), GlyphData, GL_STATIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, GlyphDataTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, GlyphTBO);
	Renderer->GlyphDataHandle = GlyphDataTexture;

	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	// NOTE: Shaders
	/*
	Renderer->BasicProgram = CreateBasicProgram(Renderer->PerFrameMemory);
	Renderer->LineProgram = CreateLineProgram(Renderer->PerFrameMemory);
    Renderer->SpriteProgram = CreateSpriteProgram(Renderer->PerFrameMemory);
	Renderer->TextProgram = CreateTextProgram(Renderer->PerFrameMemory);
	*/
	
	Renderer->BasicProgram = CreateBasicProgram(&Renderer->Memory);
	Renderer->LineProgram = CreateLineProgram(&Renderer->Memory);
    Renderer->SpriteProgram = CreateSpriteProgram(&Renderer->Memory);
	Renderer->TextProgram = CreateTextProgram(&Renderer->Memory);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

internal void DrawRenderGroup(render_state *Renderer, render_group *Group)
{
	opengl_state *State = (opengl_state*)Renderer->Backend.State;
	Assert(State != 0);

    // TODO: We always use the last PushTransform call
	// NOTE: This depends on the command right now.
    mat4 ProjectionTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    u32 SpriteVertexCount = 0;
    spritemap_vertex *SpriteVertices = 0;

    TIMED_FUNCTION();
	{ TIMED_BLOCK("Setup");

		SetDefaultGLState();
		SpriteVertices = (spritemap_vertex*)push_array(Renderer->PerFrameMemory, spritemap_vertex, MAX_NUM_SPRITE_VERTICES);
	}

	{ TIMED_BLOCK("Execute command buffer");
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
					TIMED_BLOCK("render_entry_sprite");
					if (SpriteVertexCount < MAX_NUM_SPRITE_VERTICES) {
						spritemap_vertex *Vertex = SpriteVertices + SpriteVertexCount++;
						Vertex->Position = Entry->Center;
						Vertex->Offset = Entry->Offset;
						// TODO: Premultiply alpha here or in shader?
						Vertex->Tint = PremultiplyAlpha(Entry->TintColor);
					}
				} END_CASE_AND_BREAK;

				BEGIN_CASE(render_entry_text)
				{
					TIMED_BLOCK("render_entry_text");
					// TODO: Use a common vertex buffer
					glyph_vertex *VertexBase = (glyph_vertex*)((u8*)Entry + Entry->DataHeader.Offset);
					Offset += Entry->DataHeader.Size;

					GLuint glyph_vao;
					glGenVertexArrays(1, &glyph_vao);
					glBindVertexArray(glyph_vao);

					GLuint glyph_vbo;
					glGenBuffers(1, &glyph_vbo);
					glBindBuffer(GL_ARRAY_BUFFER, glyph_vbo);
					glBufferData(GL_ARRAY_BUFFER, Entry->DataHeader.Size, VertexBase, GL_STATIC_DRAW);

					GLuint pos_id = GlobalVertexAttribs[VertexAttrib_Position].Id;
					glVertexAttribPointer(pos_id, 2, GL_FLOAT, GL_FALSE, sizeof(glyph_vertex), (void*)OffsetOf(glyph_vertex, Position));
					glEnableVertexAttribArray(pos_id);
					
					GLuint idx_id = GlobalVertexAttribs[VertexAttrib_Index].Id;
					glVertexAttribIPointer(idx_id, 1, GL_UNSIGNED_INT, sizeof(glyph_vertex), (void*)OffsetOf(glyph_vertex, Index));
					glEnableVertexAttribArray(idx_id);
					
					GLuint TextProg = Renderer->TextProgram.Id;
					glUseProgram(TextProg);

					GLint ProjectionTransformLoc = glGetUniformLocation(TextProg, "ProjectionTransform");
					glUniformMatrix4fv(ProjectionTransformLoc, 1, GL_FALSE, ProjectionTransform.ptr);
					
					GLint GlyphAtlasSizeLoc = glGetUniformLocation(TextProg, "GlyphAtlasSize");
					glUniform2i(GlyphAtlasSizeLoc, GLYPH_ATLAS_COUNT_X, GLYPH_ATLAS_COUNT_Y);
					
					GLint GlyphTileSizeLoc = glGetUniformLocation(TextProg, "GlyphTileSize");
					glUniform2i(GlyphTileSizeLoc, MAX_GLYPH_PIXELS_X, MAX_GLYPH_PIXELS_Y);
					
					glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_GLYPH_ATLAS);
					glBindTexture(GL_TEXTURE_2D, Renderer->GlyphAtlas->Handle);
					GLint GlyphAtlasTexLoc = glGetUniformLocation(TextProg, "GlyphAtlasTex");
					glUniform1i(GlyphAtlasTexLoc, TEXTURE_UNIT_GLYPH_ATLAS);
					
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					
					glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_GLYPH_DATA);
					glBindTexture(GL_TEXTURE_BUFFER, Renderer->GlyphDataHandle);
					GLint GlyphDataBufferLoc = glGetUniformLocation(TextProg, "GlyphDataBuffer");
					glUniform1i(GlyphDataBufferLoc, TEXTURE_UNIT_GLYPH_DATA);

					GLint TintLoc = glGetUniformLocation(TextProg, "GlyphColor");
					glUniform4fv(TintLoc, 1, Entry->Tint.E);

					glDrawArrays(GL_POINTS, 0, Entry->NumCharacters);

					glUseProgram(0);
					glBindVertexArray(0);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glDeleteBuffers(1, &glyph_vao);
					glDeleteVertexArrays(1, &glyph_vbo);

				} END_CASE_AND_BREAK;

				BEGIN_CASE(render_entry_lines)
				{
					TIMED_BLOCK("render_entry_lines");
					// TODO: Use a common vertex buffer
					v2 *PositionBase = (v2*)((u8*)Entry + Entry->DataHeader.Offset);
					Offset += Entry->DataHeader.Size;

					GLuint line_vao;
					glGenVertexArrays(1, &line_vao);
					glBindVertexArray(line_vao);

					GLuint line_vbo;
					glGenBuffers(1, &line_vbo);
					glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
					glBufferData(GL_ARRAY_BUFFER, Entry->DataHeader.Size, PositionBase, GL_STATIC_DRAW);

					GLuint pos_id = GlobalVertexAttribs[VertexAttrib_Position].Id;
					glVertexAttribPointer(pos_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
					glEnableVertexAttribArray(pos_id);
				
					GLuint LineProg = Renderer->LineProgram.Id;
					glUseProgram(LineProg);

					GLint ProjectionTransformLoc = glGetUniformLocation(LineProg, "ProjectionTransform");
					GLint LineWidthLoc = glGetUniformLocation(LineProg, "LineWidth");
					GLint LineColorLoc = glGetUniformLocation(LineProg, "LineColor");
					glUniformMatrix4fv(ProjectionTransformLoc, 1, GL_FALSE, ProjectionTransform.ptr);
					glUniform1f(LineWidthLoc, 1.f);
					glUniform4fv(LineColorLoc, 1, Entry->Color.E);

					glDrawArrays(GL_LINE_STRIP, 0, Entry->VertexCount);

					glUseProgram(0);
					glBindVertexArray(0);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glDeleteBuffers(1, &line_vbo);
					glDeleteVertexArrays(1, &line_vao);
				} END_CASE_AND_BREAK;
			
				BEGIN_CASE(render_entry_rect)
				{
					// TODO: Remove this ugly copy paste (render_entry_rect_outline)
					basic_vertex Vertices[4];
					v2 TexCoord[4] = {
						0, 0,
						1, 0,
						0, 1,
						1, 1
					};
					for(u32 i = 0; i < 4; i++) {
						Vertices[i].Position = Entry->Corners[i].Position;
						Vertices[i].TexCoord = TexCoord[i];
						Vertices[i].Tint = Entry->Color;
					}
					
					glBindVertexArray(State->BasicVertexArray);
					glBindBuffer(GL_ARRAY_BUFFER, State->BasicVertexBuffer);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);

					glUseProgram(Renderer->BasicProgram.Id);

					// TODO: Cache locations
					GLint ProjectionTransformLoc = glGetUniformLocation(Renderer->BasicProgram.Id, "ProjectionTransform");
					glUniformMatrix4fv(ProjectionTransformLoc, 1, GL_FALSE, ProjectionTransform.ptr);
				
					// TODO: Load a white 1x1 texture OR remove texturing from basic shader!
					GLint TexLoc = glGetUniformLocation(Renderer->BasicProgram.Id, "Tex");
					GLint TexUnit = 1;
					glUniform1i(TexLoc, TexUnit);

					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

					glBindVertexArray(0);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glUseProgram(0);
				} END_CASE_AND_BREAK;
			
				BEGIN_CASE(render_entry_rect_outline)
				{
					// TODO: Remove this ugly copy paste (render_entry_rect)
					basic_vertex Vertices[10];
					// TODO: Tex coordinates might not make much sense (proper UV)
					v2 TexCoord[10] = {};
					u32 Indices[5] = { 0, 1, 2, 3, 0 };
					for(u32 i = 0; i < 5; i++) {
						Vertices[2 * i + 0].Position = Entry->OuterCorners[Indices[i]].Position;
						Vertices[2 * i + 0].TexCoord = TexCoord[i];
						Vertices[2 * i + 0].Tint = Entry->Color;
						Vertices[2 * i + 1].Position = Entry->InnerCorners[Indices[i]].Position;
						Vertices[2 * i + 1].TexCoord = TexCoord[i];
						Vertices[2 * i + 1].Tint = Entry->Color;
					}
					
					glBindVertexArray(State->BasicVertexArray);
					glBindBuffer(GL_ARRAY_BUFFER, State->BasicVertexBuffer);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);

					glUseProgram(Renderer->BasicProgram.Id);

					// TODO: Cache locations
					GLint ProjectionTransformLoc = glGetUniformLocation(Renderer->BasicProgram.Id, "ProjectionTransform");
					glUniformMatrix4fv(ProjectionTransformLoc, 1, GL_FALSE, ProjectionTransform.ptr);
				
					// TODO: Load a white 1x1 texture OR remove texturing from basic shader!
					GLint TexLoc = glGetUniformLocation(Renderer->BasicProgram.Id, "Tex");
					GLint TexUnit = 1;
					glUniform1i(TexLoc, TexUnit);

					glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

					glBindVertexArray(0);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glUseProgram(0);
				} END_CASE_AND_BREAK;

				BEGIN_CASE(render_entry_clear)
				{
					glClearColor(Entry->ClearColor.r, Entry->ClearColor.g, Entry->ClearColor.b, Entry->ClearColor.a);
					glClear(GL_COLOR_BUFFER_BIT);
				} END_CASE_AND_BREAK;
			
				BEGIN_CASE(render_entry_transformation)
				{
					// TODO: Figure out how we want to deal with transformations (per group, transformation block, etc.)
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
	} // TIMED_BLOCK

	RenderSprites(Renderer, SpriteVertices, SpriteVertexCount, ProjectionTransform);

	{ TIMED_BLOCK("Cleanup");
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glUseProgram(0);
	}
}
