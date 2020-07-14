// NOTE: To add a particle system you need to:
// 1: Define a new type
// 2: Add a case in InitializeParticleCache
// 3: Add some way to spawn particles
// 4: Add a case in UpdateAndRenderParticles

enum particle_system_type
{
	ParticleSystemType_Explosion,
	ParticleSystemType_JetExhaust,

	ParticleSystemType_Count,
};

struct particle
{
	f32 TimeToLive;
	basis2d Basis;
	v2 P;
	v2 dP;
	v2 ddP;
	v4 C;
};

enum sprite_type;
#define MAX_PARTICLE_COUNT 1024
struct particle_system
{
	f32 ParticleLifetime;
	u32 NextParticleIndex;
	sprite_type Sprite;
	particle Particles[MAX_PARTICLE_COUNT];
};

struct particle_cache
{
	particle_system Systems[ParticleSystemType_Count];
	rand_state Entropy;
};