internal void InitializeRenderBackend(render_state *Renderer);
internal void ReloadRenderBackend();
internal texture * CreateTexture(render_state *State, u32 Width, u32 Height, u8 *Data);
internal void UpdateSpritemapSprite(spritemap_array *Spritemaps, u32 OffsetX, u32 OffsetY, u32 Index, sprite_pixel *Pixels);
internal basic_program CreateBasicProgram(mem_arena *Memory);
internal line_program CreateLineProgram();
internal sprite_program CreateSpriteProgram(mem_arena *Memory);
internal void DrawRenderGroup(render_state *Renderer, render_group *Group);