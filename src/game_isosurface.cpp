internal inline void CreateIsoSurfaceGrid(isosurface_grid *Grid, u32 GridX, u32 GridY, v2 Offset, f32 Width, mem_arena *Arena)
{
	Grid->Valid = true;
	Grid->GridX = GridX;
	Grid->GridY = GridY;
	Grid->Spacing = Width / (f32)(GridX - 1);
	Grid->Values = push_array(Arena, f32, GridX * GridY);
	// TODO: Using too much memory right now
	//Grid->Cells = push_array(Arena, u8, (GridX - 1) * (GridY - 1));
	Grid->Cells = push_array(Arena, u8, GridX * GridY);
	Grid->Offset = Offset;
}

#define ISOSPHERE_RADIUS 250.f
internal inline f32 SampleSurface(v2 *Objects, u32 NumObjects, f32 X, f32 Y)
{
	f32 Result = -1.f;
	for(u32 ObjIdx = 0; ObjIdx < NumObjects; ObjIdx++) {
		v2 *Object = Objects + ObjIdx;
		f32 Dx = Object->x - X;
		f32 Dy = Object->y - Y;
		Result += ISOSPHERE_RADIUS / (Dx * Dx + Dy * Dy + 1e-20f);
	}

	return Result;
}

internal void BuildIsoGrid(isosurface_grid *Grid, v2 *Objects, u32 NumObjects)
{
	TIMED_FUNCTION();

	if(Grid->Valid) {
		f32 SampleX, SampleY;
		f32 *Ptr = Grid->Values;
		for(u32 iy = 0; iy < Grid->GridY; iy++) {
			SampleY = Grid->Offset.y + iy * Grid->Spacing;
			for(u32 ix = 0; ix < Grid->GridX; ix++) {
				SampleX = Grid->Offset.x + ix * Grid->Spacing;
				*Ptr++ = SampleSurface(Objects, NumObjects, SampleX, SampleY);
			}
		}

		u8 *CellPtr = Grid->Cells;
		for(u32 iy = 0; iy < Grid->GridY - 1; iy++) {
			for(u32 ix = 0; ix < Grid->GridX - 1; ix++) {
				u8 SW = (Grid->Values[iy * Grid->GridX + ix] > 0.f) ? 1 : 0;
				u8 SE = (Grid->Values[iy * Grid->GridX + ix + 1] > 0.f) ? 2 : 0;
				u8 NE = (Grid->Values[(iy + 1) * Grid->GridX + ix + 1] > 0.f) ? 4 : 0;
				u8 NW = (Grid->Values[(iy + 1) * Grid->GridX + ix] > 0.f) ? 8 : 0;
				u8 Value = SW | SE | NW | NE;
				*CellPtr++ = Value;
			}
		}
	}
}

internal inline edge * AddEdge(edge_buffer *Buffer, v2 P, v2 Q)
{
	edge * Result = 0;
	if(Buffer->Count < MAX_EDGE_COUNT) {
		Result = Buffer->Edges + Buffer->Count++;
		Result->P = P;
		Result->Q = Q;
	}

	return Result;
}

// TODO: Record objects into grid so you can't accidently pass garbage data
internal void GetIsoSurfaceEdges(isosurface_grid *Grid, v2 *Objects, u32 NumObjects, edge_buffer *Buffer)
{
	TIMED_FUNCTION();

	if(Grid->Valid) {
		u8 *Ptr = Grid->Cells;
		u32 Count = 0;
		f32 h = 0.5f * Grid->Spacing;
		f32 CellX, CellY;
		for(u32 iy = 0; iy < Grid->GridY - 1; iy++) {
			CellY = Grid->Offset.y + iy * Grid->Spacing + h;
			for(u32 ix = 0; ix < Grid->GridX - 1; ix++) {
				CellX = Grid->Offset.x + ix * Grid->Spacing + h;
				v2 A, B, C, D, Center;
				Center = V2(CellX, CellY);
				A = Center + V2(-h, -h);
				B = Center + V2( h, -h);
				C = Center + V2( h,  h);
				D = Center + V2(-h,  h);
				switch(*Ptr++) {
					case 0:
					case 15:
						break;
					case 1:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, D, A);
						break;
					case 2:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, B, C);
						break;
					case 3:
						AddEdge(Buffer, D, A);
						AddEdge(Buffer, B, C);
						break;
					case 4:
						AddEdge(Buffer, B, C);
						AddEdge(Buffer, C, D);
						break;
					case 5:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, B, C);
						AddEdge(Buffer, C, D);
						AddEdge(Buffer, D, A);
						break;
					case 6:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, C, D);
						break;
					case 7:
						AddEdge(Buffer, C, D);
						AddEdge(Buffer, D, A);
						break;
					case 8:
						AddEdge(Buffer, C, D);
						AddEdge(Buffer, D, A);
						break;
					case 9:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, C, D);
						break;
					case 10:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, D, A);
						AddEdge(Buffer, B, C);
						AddEdge(Buffer, C, D);
						break;
					case 11:
						AddEdge(Buffer, B, C);
						AddEdge(Buffer, C, D);
						break;
					case 12:
						AddEdge(Buffer, B, C);
						AddEdge(Buffer, D, A);
						break;
					case 13:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, B, C);
						break;
					case 14:
						AddEdge(Buffer, A, B);
						AddEdge(Buffer, D, A);
						break;

					InvalidDefaultCase;
				}
			}
		}

		u32 EdgeCount = Buffer->Count;
		Buffer->Count = 0;
		for(u32 i = 0; i < EdgeCount / 2; i++) {
			edge *E = Buffer->Edges + 2 * i;
			
			f32 ValuesP[2];
			ValuesP[0] = SampleSurface(Objects, NumObjects, E[0].P.x, E[0].P.y);
			ValuesP[1] = SampleSurface(Objects, NumObjects, E[1].P.x, E[1].P.y);
			f32 t = -ValuesP[0] / (SampleSurface(Objects, NumObjects, E[0].Q.x, E[0].Q.y) - ValuesP[0]);
			f32 s = -ValuesP[1] / (SampleSurface(Objects, NumObjects, E[1].Q.x, E[1].Q.y) - ValuesP[1]);
			
			v2 P, Q;
			P = E[0].P + Clamp(t, 0.f, 1.f) * (E[0].Q - E[0].P);
			Q = E[1].P + Clamp(s, 0.f, 1.f) * (E[1].Q - E[1].P);
			AddEdge(Buffer, P, Q);
		}
	}
}

internal inline edge_buffer * AllocateEdgeBuffer(mem_arena *Arena)
{
	edge_buffer * Result = push_type(Arena, edge_buffer);
	if(Result) {
		Result->Count = 0;
	}

	return Result;
}

void PushEdges(render_group *Group, edge *Edges, u32 Count, v4 Color)
{
	TIMED_FUNCTION();

	for(u32 i = 0; i < Count; i++) {
		edge *Edge = Edges + i;
		PushLines(Group, &Edge->P, 2, 1.f, RenderEntryLinesMode_Simple, Color);
	}
}
#if 0
internal void DebugDrawIsoGrid(render_group *Group, isosurface_grid *Grid)
{
	TIMED_FUNCTION();

	f32 *Ptr = Grid->Values;
	sprite S = {};
	S.HalfDim = V2(0.5f, 0.5f);
	if(Grid->Valid) {
		for(u32 iy = 0; iy < Grid->GridY; iy++) {
			for(u32 ix = 0; ix < Grid->GridX; ix++) {
				f32 Value = *Ptr++;
				v4 Color = (Value < 0.f) ? V4(0.3f, 0.1f, 0.1f, 1.f) : V4(0.f, 1.f, 0.f, 1.f);
				v2 P = Grid->Offset + V2(ix * Grid->Spacing, iy * Grid->Spacing);
				PushSprite(&Batch, P, CanonicalBasis(), Color);
				/*
				rect2 R = MakeRectDim(P, V2(1.f, 1.f));
				PushRectangle(Group, R, Color);
				*/
			}
		}
	}
	PushRectangleOutline(Group, MakeRectFromCorners(Grid->Offset, Grid->Offset + V2(Grid->GridX * Grid->Spacing, Grid->GridY *Grid->Spacing)), 1.f, V4(0.f, 1.f, 0.f, 1.f));
}
#endif