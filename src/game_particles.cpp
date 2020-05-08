internal void AllocateParticleCache(game_state *State)
{
	particle_cache *Cache = push_type(&State->MemoryArena, particle_cache); 
	if(Cache) {
		Cache->Entropy = CreateRNG(7 * *(u32*)&Cache);
	}
	PushSize(&State->MemoryArena, MEGABYTES(2));

	State->ParticleCache = Cache;
}

internal void InitializeParticleCache(particle_cache *Cache)
{
	if(Cache) {
		for(u32 i = 0; i < ParticleSystemType_Count; i++) {
			particle_system *System = Cache->Systems + i;
			switch(i) {
				case ParticleSystemType_Explosion:
				{
					System->NextParticleIndex = 0;
					System->ParticleLifetime = 0.5f;
				} break;

				case ParticleSystemType_JetExhaust:
				{
					System->NextParticleIndex = 0;
					System->ParticleLifetime = 0.25f;
				} break;

				InvalidDefaultCase;
			}
		}
	}
}

internal inline particle * GetNextParticle(particle_system *System)
{
	u32 Idx = System->NextParticleIndex;
	if(Idx == MAX_PARTICLE_COUNT) {
		Idx = 0;
	}
	particle *Particle = System->Particles + Idx++;
	System->NextParticleIndex = Idx;
	return Particle;
}

internal void SpawnJetExhaust(particle_cache *Cache, v2 P, v2 JetDir, f32 ConeAngle, f32 RadialOffset, f32 MeanSpeed)
{
	f32 HalfConeAngle = 0.5f * DegreesToRadians(ConeAngle);
	if(Cache) {
		particle_system *System = &Cache->Systems[ParticleSystemType_JetExhaust];
		rand_state *Entropy = &Cache->Entropy;
		
		f32 Phi = Atan2(JetDir);
		
		for(u32 i = 0; i < 32; i++) {
			f32 Speed = MeanSpeed * (1.f + (GetUniformRandom01(Entropy) - 0.5f));
			f32 Angle = Lerp(GetUniformRandom01(Entropy), Phi - HalfConeAngle, Phi + HalfConeAngle);
			v2 Dir = V2(Cos(Angle), Sin(Angle));
	
			particle *Particle = GetNextParticle(System);
			Particle->P = P + RadialOffset * Dir;
			Particle->dP = Speed * Dir;
			Particle->ddP = V2();
			Particle->C = V4(0.f, 1.f, 1.f, 1.f);
			Particle->TimeToLive = System->ParticleLifetime * (1.f + 0.1f * (GetUniformRandom01(Entropy) - 0.5f));
		}
	}
}

internal void SpawnExplosion(particle_cache *Cache, v2 P, f32 MeanSpeed)
{
	if(Cache) {
		particle_system *System = &Cache->Systems[ParticleSystemType_Explosion];
		rand_state *Entropy = &Cache->Entropy;
	
		for(u32 i = 0; i < 128; i++) {
			particle *Particle = GetNextParticle(System);
			f32 Speed = MeanSpeed * (1.f + (GetUniformRandom01(Entropy) - 0.75f));
			f32 Angle = 2.f * PI * GetUniformRandom01(Entropy);
			Particle->P = P;
			Particle->dP = Speed * V2(Cos(Angle), Sin(Angle));
			Particle->ddP = V2();
			Particle->C = V4(1.f, 1.f, 0.f, 1.f);
			Particle->TimeToLive = System->ParticleLifetime * (1.f + 0.1f * (GetUniformRandom01(Entropy) - 0.5f));
		}
	}
}

// TODO: Do we really want to pass the camera down, or is there a better approach?
internal void UpdateAndRenderParticles(particle_cache *Cache, camera *Camera, f32 dt, render_group *Group)
{
	TIMED_FUNCTION();

    // TODO: Put this somewhere sane
	if(Cache) {
		for(u32 i = 0; i < ParticleSystemType_Count; i++) {
			particle_system *System = Cache->Systems + i;

			f32 ParticleSize = 1.f;
			basis2d Basis = ParticleSize * CanonicalBasis();
            switch(i) {
                case ParticleSystemType_Explosion:
                {
                    for(u32 i = 0; i < MAX_PARTICLE_COUNT; i++) {
                        particle *Particle = System->Particles + i;
                        Particle->P += 0.5f * Particle->ddP * dt * dt + (Particle->dP - Camera->Velocity) * dt;
                        Particle->dP += Particle->ddP * dt;
                        Particle->ddP = -Particle->dP;
                        Particle->TimeToLive -= dt;
                        Particle->C.a = Lerp(Clamp(Particle->TimeToLive / System->ParticleLifetime, 0.f, 1.f), 0.f, 1.f);
                        Particle->C.g *= Sqrt(Particle->C.a);

                        PushSprite(Group, GetLayer(LAYER_BACKGROUND), Particle->P, SPRITE_EXPLOSION, Basis, Particle->C);
                    }
                } break;
                case ParticleSystemType_JetExhaust:
                {
					v4 EndColor = V4(0.4f, 0.2f, 0.f, 0.f);
                    for(u32 i = 0; i < MAX_PARTICLE_COUNT; i++) {
                        particle *Particle = System->Particles + i;
                        f32 t = Clamp(Particle->TimeToLive / System->ParticleLifetime, 0.f, 1.f);
                        Particle->P += (Particle->dP - Camera->Velocity) * dt;
                        Particle->TimeToLive -= dt;
                        Particle->C = Lerp(Sqrt(t), EndColor, Particle->C);
                        basis2d B = ScaleBasis(Basis, t, t);

                        PushSprite(Group, GetLayer(LAYER_BACKGROUND), Particle->P, SPRITE_JET_EXHAUST, Basis, Particle->C);
                    }
                } break;

                InvalidDefaultCase;
            }
		}
	}
}