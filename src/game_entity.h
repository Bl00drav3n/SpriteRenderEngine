struct entity_id
{
	u32 Value;
};

internal inline entity_id InvalidEntityId()
{
	entity_id Result = {};
	return Result;
}

enum sim_entity_flag
{
	SimEntityFlag_Delete = 0x1
};

enum entity_type
{
	EntityType_Null,

	EntityType_Player,
	EntityType_Enemy,
	EntityType_Projectile,
	EntityType_Floater,
	EntityType_Item,
	EntityType_Shield,
};

struct game_state;
struct level;
struct sim_region;
struct sim_entity;
#define ENTITY_TICK_DISPATCH(name) void name(game_state *State, game_inputs *Inputs, sim_region *Region, camera *Camera, level *Level, sim_entity *Entity, f32 dt)
typedef ENTITY_TICK_DISPATCH(tick_dispatch);

// TODO: Find a way to sync sim_entity and stored_entity
// Common part on top, specialization at address after Shared?
struct sim_entity
{
	entity_id        Id;
	entity_id        Owner;
	entity_type      Type;
	v2               Velocity;
	sprite_type      Sprite;
	collision_volume Col;
	f32              Speed;
	f32              tParam;
	faction_type     Faction;
	s32              Health;
	item_type        ItemType;
	tick_dispatch *  Tick;
	
	weapon           Weapon;
	s32              DamagePerHit;
	v2				 LastPosition;
	v2               NextPosition;
	u32              Flags;
    tex_coords       TexCoords;
};

struct stored_entity
{	
	entity_id        Id;
	entity_id        Owner;
	entity_type      Type;
	entity_type      OwnerType;
	v2               Velocity;
	sprite_type      Sprite;
	collision_volume Col;
	f32              Speed;
	f32              tParam;
	faction_type     Faction;
	u32              Health;
	item_type        ItemType;
	tick_dispatch *  Tick;		// NOTE: Do we need to save this?
	
	weapon           Weapon;
	s32              DamagePerHit;
	world_position   Position;
};

// NOTE: Every entity data block shares this information
struct entity_data_header
{
	world_position Pos;
	faction_type   Faction;
};

// TODO: Systemize entity structures

internal inline umm GetEntityDataSize(stored_entity *Entity)
{
	// TODO: This needs to be implemented once we have useful entity representations
	umm Result;
	switch(Entity->Type) {
		case 0:
			Result = 0;
			break;
		default:
			Result = sizeof(entity_data_header);
	}

	return Result;
}