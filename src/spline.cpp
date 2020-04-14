#define NATURAL_BITFIELD 0x7F800001

// TODO: Do we really want this? Better solution?
union derivative
{
	f32 value;
	u32 bitfield;
};

internal inline derivative natural_bounding_cond()
{
	derivative Result;
	Result.bitfield = NATURAL_BITFIELD;

	return Result;
}

internal void build_spline(f32 *x, f32 *y, f32 *y2, u32 n, derivative yp1 = natural_bounding_cond(), derivative ypn = natural_bounding_cond())
{
	f32 sig, p, qn, un;
	f32 *u = (f32*)PushAlloc(n * sizeof(*u));

	// NOTE: Boundary condition: natural if yp1 >= 1e30
	if(yp1.bitfield == NATURAL_BITFIELD) {
		y2[0] = u[0] = 0.f;
	}
	else {
		y2[0] = -0.5f;
		u[0] = (3.f / (x[1] - x[0])) * ((y[1] - y[0]) / (x[1] - x[0]) - yp1.value);
	}

	// NOTE: Decomposition of tridiagonal algorithm
	for(u32 i = 1; i < n - 1; i++) {
		sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
		p = sig * y2[i - 1] + 2.f;
		y2[i] = (sig - 1.f) / p;
		u[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]);
		u[i] = (6.f * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
	}

	// NOTE: Boundary condition: natural if ypn >= 1e30
	if(ypn.bitfield == NATURAL_BITFIELD) {
		qn = un = 0.f;
	}
	else {
		qn = 0.5f;
		un = (3.f / (x[n - 1] - x[n - 2])) * (ypn.value - (y[n - 1] - y[n - 2]) / (x[n - 1] - x[n - 2]));
	}

	// NOTE: Backsubstition of tridiagonal algorithm
	y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.f);
	for(s32 k = n - 2; k >= 0; k--) {
		y2[k] = y2[k] * y2[k + 1] + u[k];
	}

	PopAlloc();
}

internal void cubic_interp(f32 *xa, f32 *ya, f32 *y2a, u32 n, f32 x, f32 *y)
{
	u32 klo = 0;
	u32 khi = n - 1;

	// NOTE: Bisection
	u32 k;
	while(khi - klo > 1) {
		k = (khi + klo) >> 1;
		if(xa[k] > x)
			khi = k;
		else
			klo = k;
	}

	f32 h = xa[khi] - xa[klo];
	f32 a = (xa[khi] - x) / h;
	f32 b = (x - xa[klo]) / h;
	*y = a * ya[klo] + b * ya[khi] + ((a * a * a - a) * y2a[klo] + (b * b * b - b) * y2a[khi]) * (h * h) / 6.f;
}

internal inline void BakeParametricSpline(parametric_spline *S)
{
	if(S->Count > 1) {
		build_spline(S->t, S->x, S->x2, S->Count);
		build_spline(S->t, S->y, S->y2, S->Count);
	}
}

internal inline v2 SplineAtTime(parametric_spline *S, f32 t)
{
	v2 Result;
	t = Clamp(t, 0.f, (f32)S->Count);
	cubic_interp(S->t, S->x, S->x2, S->Count, t, &Result.x);
	cubic_interp(S->t, S->y, S->y2, S->Count, t, &Result.y);

	return Result;
}

internal void PushSplinePoint(parametric_spline *S, v2 P)
{
	if(S->Count < MAX_SPLINE_CTRL_PTS) {
		S->t[S->Count] = (f32)S->Count;
		S->x[S->Count] = P.x;
		S->y[S->Count] = P.y;
		S->Count++;
		BakeParametricSpline(S);
	}
}

internal void PopSplinePoint(parametric_spline *S)
{
	if(S->Count) {
		S->Count--;
		if(S->Count > 0) {
			BakeParametricSpline(S);
		}
	}
}