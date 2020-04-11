// TODO: Create Sprite Management system O_O
sprite CreateFloaterTestSprite() {
    return CreateSprite(0, 0, 0);
}
sprite CreatePlayerTestSprite() {
    return CreateSprite(1, 0, 0);
}
sprite CreateEnemyTestSprite() {
    return CreateSprite(2, 0, 0);
}
sprite CreateItemTestSprite() {
    return CreateSprite(3, 0, 0);
}
sprite CreateShieldTestSprite() {
    return CreateSprite(4, 0, 0);
}
sprite CreateProjectileTestSprite() {
    return CreateSprite(5, 0, 0);
}
sprite CreateSpreadShotTestSprite() {
    return CreateSprite(6, 0, 0);
}
sprite CreateEnemySingleShotTestSprite() {
    return CreateSprite(7, 0, 0);
}
sprite CreateFloaterSingleShotTestSprite() {
    return CreateSprite(8, 0, 0);
}

internal inline void FlagForRemoval(sim_entity *Entity)
{
	Entity->Flags |= SimEntityFlag_Delete;
}

internal inline b32 IsFlagSet(sim_entity *Entity, sim_entity_flag Flag)
{
	b32 Result = (Entity->Flags & Flag);

	return Result;
}

internal inline u32 HashEntityId(entity_id Id)
{
	u32 Hash = Id.Value % MAX_SIM_ENTITY_COUNT;
	return Hash;
}

internal sim_entity * AllocateSimEntity(sim_region *Region, entity_id Id)
{
	TIMED_FUNCTION();

	Assert(Id.Value);
	sim_entity *Result = 0;	
	u32 HashIndex = HashEntityId(Id);
	for(u32 i = 0; i < MAX_SIM_ENTITY_COUNT; i++) {
		sim_entity *TestEntity = Region->EntityTable + ((HashIndex + i) % MAX_SIM_ENTITY_COUNT);
		if(TestEntity->Id.Value == 0) {
			Result = TestEntity;
			break;
		}
	}

	if(Result) {
#if 0
		ClearMemoryToZero(Result, sizeof(sim_entity));
#endif

		// NOTE: Insert into list
		sim_entity_item *Item = push_type(Region->TransMemory, sim_entity_item);
		Item->Entity = Result;

		sim_entity_item *Next = Region->EntityListSentry.Next;
		Item->Next = Next;
		Region->EntityListSentry.Next = Item;
	}

	return Result;
}

internal inline entity_id GetUniqueEntityId(level *Level)
{
	entity_id Result;
	Assert(Level->RunningEntityIndex != 0);

	u32 UID = Level->RunningEntityIndex++;
	Result.Value = UID;

	return Result;
}

internal inline sim_entity * CreateSimEntity(sim_region *Region, level *Level)
{
	entity_id Id = GetUniqueEntityId(Level);
	sim_entity *Result = AllocateSimEntity(Region, Id);
	if(Result) {
		Result->Id = Id;
		Region->DebugLoadedEntityCount++;
	}

	return Result;
}

internal stored_entity * CreateStoredEntity(level *Level, u32 BlockIndex)
{
	stored_entity *Result = 0;
	level_block *Block = GetLevelBlockFromIndex(Level, BlockIndex);
	if(Block) {
		if(Block->EntityCount < MAX_ENTITIES_PER_BLOCK) {
			Result = Block->Entities + Block->EntityCount++;
			ZeroStruct(Result);
			Result->Id = GetUniqueEntityId(Level);
			Result->tParam = 0.f;
		}
	}
	else {
		InvalidCodepath;
	}

	return Result;
}

internal sim_entity * GetSimEntityById(sim_region *Region, entity_id Id)
{
	TIMED_FUNCTION();

	sim_entity *Result = 0;

	if(Id.Value) {
		u32 HashIndex = HashEntityId(Id);
		for(u32 i = 0; i < MAX_SIM_ENTITY_COUNT; i++) {
			sim_entity *TestEntity = Region->EntityTable + ((HashIndex + i) % MAX_SIM_ENTITY_COUNT);
			if(TestEntity->Id.Value == 0)
				break;
			if(TestEntity->Id.Value == Id.Value) {
				Result = TestEntity;
				break;
			}
		}
	}

	return Result;
}

internal void StoreEntity(game_state *State, sim_entity *SimEntity)
{
	TIMED_FUNCTION();

	Assert(SimEntity->Type);

	level *Level = &State->Level;
	if(!IsFlagSet(SimEntity, SimEntityFlag_Delete)) {
		world_position P = GetCanonicalWorldPosition(Level, SimEntity->NextPosition);
		level_block *Block = GetLevelBlockFromIndex(Level, P.BlockIndex);
		if(Block) {
			if(Block->EntityCount < MAX_ENTITIES_PER_BLOCK) {
				stored_entity *Entity = Block->Entities + Block->EntityCount++;
				Entity->Id = SimEntity->Id;
				Entity->Owner = SimEntity->Owner;
				Entity->Type = SimEntity->Type;
				Entity->Position = P;
				Entity->Velocity = SimEntity->Velocity;
				Entity->Sprite = SimEntity->Sprite;
				Entity->Speed = SimEntity->Speed;
				Entity->Col = SimEntity->Col;
				Entity->tParam = SimEntity->tParam;
				Entity->Faction = SimEntity->Faction;
				Entity->Health = SimEntity->Health;
				Entity->ItemType = SimEntity->ItemType;
				Entity->Weapon = SimEntity->Weapon;
				Entity->DamagePerHit = SimEntity->DamagePerHit;
				Entity->Tick = SimEntity->Tick;
			}
			else {
				Assert(!"Entity lost");
			}
		}
		else {
			InvalidCodepath;
		}
	}
}

internal void LoadEntity(game_state *State, stored_entity *StoredEntity)
{
	TIMED_FUNCTION();

	Assert(StoredEntity->Type);

	level *Level = &State->Level;
	sim_region *Region = &State->SimRegion;
	sim_entity *SimEntity = AllocateSimEntity(Region, StoredEntity->Id);
	if(SimEntity) {
		// TODO: Handle union or use sim_entity = stored_entity?
		SimEntity->Id = StoredEntity->Id;
		SimEntity->Owner = StoredEntity->Owner;
		SimEntity->Type = StoredEntity->Type;
		SimEntity->Sprite = StoredEntity->Sprite;
		SimEntity->Col = StoredEntity->Col;
		SimEntity->Speed = StoredEntity->Speed;
		SimEntity->LastPosition = MapIntoCameraSpace(Level, StoredEntity->Position);
		SimEntity->NextPosition = SimEntity->LastPosition;
		SimEntity->Velocity = StoredEntity->Velocity;
		SimEntity->tParam = StoredEntity->tParam;
		SimEntity->Faction = StoredEntity->Faction;
		SimEntity->Health = StoredEntity->Health;
		SimEntity->ItemType = StoredEntity->ItemType;
		SimEntity->Weapon = StoredEntity->Weapon;
		SimEntity->DamagePerHit = StoredEntity->DamagePerHit;
		SimEntity->Tick = StoredEntity->Tick;
		SimEntity->Flags = 0;
		Region->DebugLoadedEntityCount++;
	}
	else {
		Assert(!"Entity lost");
	}
}

internal weapon CreateWeapon(weapon_type Type)
{
	weapon Result = {};
	Result.Type = Type;
	switch(Type) {
		case WeaponType_SpreadShot:
		{
			Result.Cooldown = 1.f / 6.f;
			Result.LaunchSpeed = 120.f;
		} break;
		case WeaponType_EnemySingleShot:
		{
			Result.Cooldown = 2.5f;
			Result.LaunchSpeed = 120.f;
		} break;
		case WeaponType_FloaterSingleShot:
		{
			Result.Cooldown = 1.f / 8.f;
			Result.LaunchSpeed = 225.f;
		} break;

		InvalidDefaultCase;
	}

	return Result;
}

internal ENTITY_TICK_DISPATCH(TickProjectile)
{	
	//ToDo: Add player facing direction
	Entity->NextPosition = Entity->LastPosition + Entity->Velocity * dt;

	v2 ViewP = Entity->NextPosition;
	if(ViewP.x < -150.f || ViewP.x > 150.f
		|| ViewP.y < -150.f || ViewP.y > 150.f)
	{
		FlagForRemoval(Entity);
	}
}

// NOTE: Velocity relative to global frame
internal void SpawnProjectile(level *Level, sim_region *Region, v2 SpawnPosition, v2 Velocity, projectile *Projectile, sim_entity *Ref)
{
    // TODO: Use sprite as parameter?
	sim_entity *Entity = CreateSimEntity(Region, Level);
	// TODO: Should projectile spawn be delayed for a frame?
	Entity->LastPosition = SpawnPosition;
	Entity->NextPosition = SpawnPosition;
	Entity->Velocity = Level->Camera.Velocity + Velocity;
	Entity->Sprite = Projectile->Sprite;
	Entity->DamagePerHit = Projectile->Damage;
	Entity->Col = Projectile->Collision;
	Entity->Type = EntityType_Projectile;
	Entity->Owner = Ref->Id;
	Entity->Faction = Ref->Faction;
	Entity->Flags = 0;
	Entity->Tick = TickProjectile;
}
	
internal void FireWeapon(game_state *State, sim_entity *Entity, v2 MuzzleP)
{
	TIMED_FUNCTION();

	level *Level = &State->Level;
	sim_region *Region = &State->SimRegion;
	weapon * Weapon = &Entity->Weapon;
	Weapon->tCooldown = Weapon->Cooldown;
	switch(Weapon->Type) {
		case WeaponType_SpreadShot:
		{	
			u32 SpreadShotCount = 5;
			f32 SpreadAngle = 30.f;
			f32 dAngle = DegreesToRadians(SpreadAngle / (f32)(SpreadShotCount - 1));
			f32 AngleStart = Atan2(Weapon->MuzzleDir) - DegreesToRadians(0.5f * SpreadAngle);

			projectile Projectile;
			Projectile.Collision.Type = CollisionType_Sphere;
			Projectile.Collision.Radius = 0.5f;
			Projectile.Damage = 1;
            Projectile.Sprite = CreateSpreadShotTestSprite();
			for(u32 i = 0; i < SpreadShotCount; i++) {
				f32 Angle = AngleStart + i * dAngle;
				v2 Dir = V2(Cos(Angle), Sin(Angle));
				v2 LaunchVelocity = Weapon->LaunchSpeed * Dir;
				SpawnProjectile(Level, Region, MuzzleP, LaunchVelocity, &Projectile, Entity);
			}
		} break;
		case WeaponType_EnemySingleShot:
		{
			projectile Projectile;
			Projectile.Collision.Type = CollisionType_Sphere;
			Projectile.Collision.XYExtendFromOrigin = V2(0.5f, 0.5f);
			Projectile.Damage = 1;
            Projectile.Sprite = CreateEnemySingleShotTestSprite();
			v2 LaunchVelocity = Weapon->LaunchSpeed * Weapon->MuzzleDir;
		
			SpawnProjectile(Level, Region, MuzzleP, LaunchVelocity, &Projectile, Entity);
		} break;
		case WeaponType_FloaterSingleShot:
		{
			projectile Projectile;
			Projectile.Collision.Type = CollisionType_Sphere;
			Projectile.Collision.Radius = 0.5f;
			Projectile.Damage = 1;
			Projectile.Sprite = CreateFloaterSingleShotTestSprite();
			v2 LaunchVelocity = Weapon->LaunchSpeed * Weapon->MuzzleDir;

			SpawnProjectile(Level, Region, MuzzleP, LaunchVelocity, &Projectile, Entity);
		} break;
		InvalidDefaultCase;
	}
}

internal ENTITY_TICK_DISPATCH(TickPlayer)
{
	Entity->Weapon.MuzzleDir = V2(1.f, 0.f);

	u32 EntitySpriteOffset = 0;
	v2 MoveVec = V2();
	if(Inputs->State[BUTTON_LEFT]) {
		MoveVec.x = -1.f;
		if(!Inputs->State[BUTTON_RIGHT])
			EntitySpriteOffset = 1;
	}
	if(Inputs->State[BUTTON_RIGHT]) {
		MoveVec.x = 1.f;
		if(!Inputs->State[BUTTON_LEFT])
			EntitySpriteOffset = 2;
	}
	if(Inputs->State[BUTTON_UP]) {
		MoveVec.y = 1.f;
		if(!Inputs->State[BUTTON_DOWN])
			EntitySpriteOffset = 3;
	}
	if(Inputs->State[BUTTON_DOWN]) {
		MoveVec.y = -1.f;
		if(!Inputs->State[BUTTON_UP])
			EntitySpriteOffset = 4;
	}
				
	v2 Velocity = Entity->Speed * Normalize(MoveVec);
#if 1
	SpawnJetExhaust(State->ParticleCache, Entity->LastPosition, V2(-1.f, 0.f), 15.f, 4.f, 150.f);
#endif
	Entity->Velocity = Camera->Velocity + Velocity;

	// TODO: Pull out into own function?
	weapon *Weapon = &Entity->Weapon;
	Weapon->tCooldown = ClampAboveZero(Weapon->tCooldown - dt);
	if(Weapon->tCooldown == 0.f && Inputs->State[BUTTON_ACTION])
	{
		FireWeapon(State, Entity, Entity->LastPosition + V2(10.f, 0.f));
	}

	// NOTE: Example usage of tiled textures
	tex_coords SubTextures[5] = {
		DefaultTexCoords(),
		{ V2(0.0f, 0.0f), V2(0.5f, 0.5f) }, { V2(0.5f, 0.0f), V2(1.0f, 0.5f) },
		{ V2(0.0f, 0.5f), V2(0.5f, 1.0f) }, { V2(0.5f, 0.5f), V2(1.0f, 1.0f) },
	};
	Entity->TexCoords = SubTextures[EntitySpriteOffset];
}

internal ENTITY_TICK_DISPATCH(TickEnemy)
{
	TIMED_FUNCTION();

	Entity->Weapon.MuzzleDir = V2(-1.f, 0.f);
	f32 AngularSpeed = 0.75f;
	f32 Angle = 2.f * PI * Entity->tParam;
	f32 Radius = 50.f;
	Entity->tParam += AngularSpeed * dt;
	Entity->Velocity = Radius * V2(0.f, Cos(Angle));
	if(Entity->tParam > 1.f)
		Entity->tParam -= 1.f;
				
	weapon *Weapon = &Entity->Weapon;
	Weapon->tCooldown = ClampAboveZero(Weapon->tCooldown - dt);
	if(Weapon->tCooldown == 0.f) {
		FireWeapon(State, Entity, Entity->LastPosition);
	}
}

internal ENTITY_TICK_DISPATCH(TickFloater)
{
	Entity->Weapon.MuzzleDir = V2(1.f, 0.f);
	sim_entity *Owner = GetSimEntityById(Region, Entity->Owner);
	if(Owner) {
		weapon *Weapon = &Entity->Weapon;

		// TODO: This might be problematic when order of code executions (owner/floater) is not always the same!
		f32 AngularSpeed = 0.5f;
		f32 Angle = 2.f * PI * Entity->tParam;
		f32 Radius = 15.f;
		Entity->LastPosition = Owner->LastPosition;
		Entity->Velocity = (Radius / dt) * V2(Cos(Angle), Sin(Angle));
		Entity->tParam += AngularSpeed * dt;
		while(Entity->tParam > 1.f) {
			Entity->tParam -= 1.f;
		}

		v2 MuzzleP = Entity->LastPosition + Entity->Velocity * dt + V2(5.f, 0.f);

		b32 ShouldFire = 0;
		if(Owner->Type == EntityType_Player) {
			if(Inputs->State[BUTTON_ACTION]) {
				ShouldFire = 1;
			}
		}
		else {
			sim_entity *Player = GetSimEntityById(Region, State->PlayerRef);
			if(Player) {
				Weapon->MuzzleDir = Normalize(Player->LastPosition - MuzzleP);
				ShouldFire = 1;
			}
		}
					
		// TODO: Compress
		Weapon->tCooldown = ClampAboveZero(Weapon->tCooldown - dt);
		if(Weapon->tCooldown == 0.f && ShouldFire) {
			// TODO: We need the next position to calculate the launch position of the projectile, but we calculate it only after the fact.
			// This might mean we want to delay the projectile spawn one frame and spawn it in the second pass below.
			FireWeapon(State, Entity, MuzzleP);
		}
	}
	else {
		FlagForRemoval(Entity);
	}
}

internal ENTITY_TICK_DISPATCH(TickShield)
{
	sim_entity *Owner = GetSimEntityById(Region, Entity->Owner);
	if(Owner) {
		Entity->LastPosition = Owner->LastPosition;
		Entity->Velocity = Owner->Velocity;
	}
	else {
		FlagForRemoval(Entity);
	}
}

internal ENTITY_TICK_DISPATCH(TickItem)
{
	Entity->NextPosition = Entity->LastPosition;
	Entity->Velocity = V2();
}

// TODO: Why two similar SpawnFloater???
internal void SpawnFloater(game_state *State, sim_region *Region, sim_entity *Ref)
{
	level *Level = &State->Level;
	sim_entity *Floater = CreateSimEntity(Region, Level);
	if(Floater) {
		collision_volume col;
		col.Type = CollisionType_Sphere;
		col.Radius = 1.5f;

		Floater->Type = EntityType_Floater;
		Floater->LastPosition = Ref->LastPosition + V2(0.f, 10.f);
		Floater->Col = col;
        Floater->Sprite = CreateFloaterTestSprite();
		Floater->Owner = Ref->Id;
		Floater->tParam = 0.f;
		Floater->Faction = Ref->Faction;
		Floater->Health = 4;
		Floater->Weapon = CreateWeapon(WeaponType_FloaterSingleShot);
		Floater->Tick = TickFloater;
	}
}

internal stored_entity * SpawnFloater(game_state *State, stored_entity *Ref)
{
	level *Level = &State->Level;
	stored_entity *Floater = CreateStoredEntity(Level, Ref->Position.BlockIndex);
	if(Floater) {
		collision_volume col;
		col.Type = CollisionType_Sphere;
		col.Radius = 1.5f;

		Floater->Type = EntityType_Floater;
		Floater->Position = Ref->Position;
		Floater->Position.Offset += V2(0.f, 10.f);
		Floater->Col = col;
		Floater->Sprite = CreateFloaterTestSprite();
		Floater->Owner = Ref->Id;
		Floater->tParam = 0.f;
		Floater->Faction = Ref->Faction;
		Floater->Health = 4;
		Floater->Weapon = CreateWeapon(WeaponType_FloaterSingleShot);
		Floater->Tick = TickFloater;
	}

	return Floater;
}

internal entity_id SpawnEnemy(game_state *State, world_position P)
{
	TIMED_FUNCTION();

	entity_id Result = {};
	level *Level = &State->Level;

	stored_entity *Enemy = CreateStoredEntity(Level, P.BlockIndex);
	if(Enemy) {
		collision_volume col;
		col.Type = CollisionType_Sphere;
		col.Radius = 2.f;
        sprite EnemySprite = CreateEnemyTestSprite();
		Enemy->Type = EntityType_Enemy;
		Enemy->OwnerType = EntityType_Enemy;
		Enemy->Position = P;
		Enemy->Col = col;
		Enemy->Velocity = V2();
		Enemy->Sprite = EnemySprite;
		Enemy->Faction = FactionType_Enemy;
		Enemy->tParam = GetUniformRandom01(&State->Entropy);
		Enemy->Health = 1;
		Enemy->Weapon = CreateWeapon(WeaponType_EnemySingleShot);
		Enemy->Tick = TickEnemy;

		stored_entity *Floater = SpawnFloater(State, Enemy);
		if(Floater) {
			Floater->Weapon.Cooldown = 2.f;
			Floater->Weapon.LaunchSpeed *= 0.5f;
			Floater->Weapon.tCooldown = GetUniformRandom01(&State->Entropy);
		}

		Result = Enemy->Id;
	}

	return Result;
}

internal void SpawnPlayer(game_state *State, sim_region *Region, v2 P)
{
	level *Level = &State->Level;
	sim_entity *Player = CreateSimEntity(Region, Level);
	if(Player) {
		collision_volume col;
		col.Type = CollisionType_Box;
		col.XYExtendFromOrigin = V2(4.f, 2.f);

		Player->Type = EntityType_Player;
		Player->LastPosition = P;
		Player->Speed = 60.f;
		Player->Col = col;
        Player->Sprite = CreatePlayerTestSprite();
		Player->Faction = FactionType_Player;
		Player->Health = 16;
		Player->Weapon = CreateWeapon(WeaponType_SpreadShot);
		Player->Tick = TickPlayer;

		SpawnFloater(State, Region, Player);
		
		State->PlayerRef = Player->Id;
	}
}

internal void SpawnShield(game_state *State, sim_region *Region, sim_entity *Owner)
{
	level *Level = &State->Level;
	sim_entity *Shield = CreateSimEntity(Region, Level);
	if(Shield) {
		v2 HalfDim = V2(5.f, 3.f);
		Shield->Type = EntityType_Shield;
		Shield->Col = MakeBoxCollision(HalfDim);
		Shield->LastPosition = Owner->NextPosition;
		Shield->NextPosition = Owner->NextPosition;
		Shield->Faction = Owner->Faction;
		Shield->Owner = Owner->Id;
		Shield->Health = 8;
        Shield->Sprite = CreateShieldTestSprite();
		Shield->Tick = TickShield;
	}
}

internal void SpawnItem(game_state *State, sim_region *Region, v2 P, item_type Type)
{	
	level *Level = &State->Level;
	sim_entity *Item = CreateSimEntity(Region, Level);
	if(Item) {
		Item->Type = EntityType_Item;
		Item->Faction = FactionType_Collectible;
		Item->LastPosition = P;
		Item->NextPosition = P;
        Item->Sprite = CreateItemTestSprite();
		Item->Col = MakeSphereCollision(0.5f);
		Item->Tick = TickItem;
	}
}