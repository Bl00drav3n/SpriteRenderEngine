struct edge
{
	v2 P;
	v2 Q;
};

#define MAX_EDGE_COUNT 4096
struct edge_buffer
{
	u32 Count;
	edge Edges[MAX_EDGE_COUNT];
};

struct isosurface_grid
{
	v2 Offset;

	b32 Valid;
	f32 Spacing;

	u32 GridX;
	u32 GridY;

	f32 *Values;
	u8 *Cells;
};
