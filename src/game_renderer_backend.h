#ifndef __GAME_RENDERER_BACKEND_H__
#define __GAME_RENDERER_BACKEND_H__

internal void           GfxInitializeRenderBackend(render_state *Renderer);
internal void           GfxReloadRenderBackend();
// TODO: We should not return a pointer in case allocation fails!
internal texture *      GfxCreateTexture(render_state *State, u32 Width, u32 Height, u8 *Data);
internal void           GfxUpdateSpritemapSprite(spritemap_array *Spritemaps, u32 SpriteType, sprite_pixel *Pixels);
internal basic_program  GfxCreateBasicProgram(mem_arena *Memory);
internal line_program   GfxCreateLineProgram();
internal sprite_program GfxCreateSpriteProgram(mem_arena *Memory);
internal text_program   GfxCreateTextProgram(mem_arena *Memory);
internal void           GfxDrawRenderGroup(render_state *Renderer, render_group *Group);

#endif