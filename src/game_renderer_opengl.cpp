#include "game_opengl.h"

enum vertex_attrib_id
{
    VertexAttrib_Position,
    VertexAttrib_TexCoord,
    VertexAttrib_SpriteOffset,
    VertexAttrib_Tint,

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
    { VertexAttrib_Tint, "VertexSpriteTint"},
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

internal spritemap_array AllocateSpritemaps()
{
    spritemap_array Result = {};

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

        Result.TexId = TexId;
        Result.TextureWidth = TextureWidth;
        Result.TextureHeight = TextureHeight;
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

internal GLuint CreateShaderProgram(char *SrcVS, char *SrcGS, char *SrcFS)
{
    GLuint Program = glCreateProgram();

    char *Sources[3] = { SrcVS, SrcGS, SrcFS };
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

internal sprite_program CreateSpriteProgram(char *VS, char *GS, char *FS)
{
    sprite_program Result;
    u32 Program = CreateShaderProgram(VS, GS, FS);
    Result.Id = Program;

    return Result;
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
				TIMED_BLOCK("render_entry_sprite");
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
				TIMED_BLOCK("render_entry_glyph");
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
				TIMED_BLOCK("render_entry_lines");
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
				TIMED_BLOCK("render_entry_rect");
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
				TIMED_BLOCK("render_entry_rect_outline");
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
				TIMED_BLOCK("render_entry_clear");
				glClearColor(Entry->ClearColor.r, Entry->ClearColor.g, Entry->ClearColor.b, Entry->ClearColor.a);
				glClear(GL_COLOR_BUFFER_BIT);
			} END_CASE_AND_BREAK;
			
			BEGIN_CASE(render_entry_transformation)
			{
				TIMED_BLOCK("render_entry_transformation");
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
				
				TIMED_BLOCK("render_entry_blend");
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

	{
		TIMED_BLOCK("Draw Sprites");
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
		
		GLint ProjectionTransformLoc = glGetUniformLocation(SpriteProg->Id, "ProjectionTransform");
		GLint SpriteHalfDimLoc = glGetUniformLocation(SpriteProg->Id, "SpriteHalfDim");
		GLint SpriteTexDimLoc = glGetUniformLocation(SpriteProg->Id, "SpriteTexDim");
		GLint SpritemapsLoc = glGetUniformLocation(SpriteProg->Id, "Spritemaps");

		glUniform1i(SpritemapsLoc, TexUnit);
		glUniformMatrix4fv(ProjectionTransformLoc, 1, GL_FALSE, ProjectionTransform.ptr);
		glUniform2fv(SpriteHalfDimLoc, 1, SpriteHalfDim.E);
		glUniform2fv(SpriteTexDimLoc, 1, SpriteTexDim.E);

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
	}

    // TODO: Keep vertex specification around (vertex array)
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &VertexBuffer);
    glDeleteVertexArrays(1, &VertexArray);
}
