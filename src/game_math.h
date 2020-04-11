#ifndef __GAME_MATH_H__
#define __GAME_MATH_H__

#include <math.h>

#define PI 3.1415926535897932384626433832795f
union v2
{
	struct {
		f32 x;
		f32 y;
	};

	f32 E[2];
};

internal inline f32 Lerp(f32 t, f32 A, f32 B)
{
	f32 Result = (1.f - t) * A + t * B;
	
	return Result;
}

internal inline f32
Sqrt(f32 Arg)
{
    f32 Result = sqrtf(Arg);

    return Result;
}

internal inline f32
Sin(f32 Angle)
{
    f32 Result = sinf(Angle);

    return Result;
}

internal inline f32
ASin(f32 Arg)
{
    f32 Result = asinf(Arg);

    return Result;
}

internal inline f32
Cos(f32 Angle)
{
    f32 Result = cosf(Angle);

    return Result;
}

internal inline f32
ACos(f32 Arg)
{
    f32 Result = acosf(Arg);

    return Result;
}

internal inline f32
Tan(f32 Angle)
{
    f32 Result = tanf(Angle);

    return Result;
}

internal inline f32
Atan2(f32 y, f32 x)
{
    f32 Result = atan2f(y, x);

    return Result;
}

internal inline f32
Atan2(v2 v)
{
    f32 Result = Atan2(v.y, v.x);

    return Result;
}

internal inline f32 
SafeDivideN(f32 p, f32 q, f32 N)
{
	f32 Result;

	if (q == N)
		Result = N;
	else
		Result = p / q;

	return Result;
}

internal inline f32
SafeDivide0(f32 p, f32 q)
{
	f32 Result = SafeDivideN(p, q, 0);

	return Result;
}

internal inline f32
SafeDivide1(f32 p, f32 q)
{
	f32 Result = SafeDivideN(p, q, 1);

	return Result;
}

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

internal inline f32
Minimum(f32 a, f32 b)
{
	f32 Result = MINIMUM(a, b);
	return Result;
}

internal inline f32
Maximum(f32 a, f32 b)
{
	f32 Result = MAXIMUM(a, b);

	return Result;
}

internal inline f32
Clamp(f32 Value, f32 Min, f32 Max)
{
	f32 Result = Maximum(Min, Minimum(Max, Value));

	return Result;
}

internal inline f32
Clamp01(f32 Value)
{
	f32 Result = Clamp(Value, 0.f, 1.f);

	return Result;
}

internal inline f32
ClampAboveZero(f32 Value)
{
	f32 Result = Maximum(Value, 0.f);

	return Result;
}

internal inline f32
Abs(f32 Value)
{
	f32 Result = fabsf(Value);

	return Result;
}

internal inline f32
Square(f32 Value)
{
	f32 Result = Value * Value;

	return Result;
}

/* === v2 === */

internal inline v2
V2(void)
{
	v2 Result = { 0, 0 };

	return Result;
}

internal inline v2
V2(f32 x, f32 y)
{
	v2 Result;
	Result.x = x;
	Result.y = y;

	return Result;
}

internal inline v2
V2(f32 value)
{
	v2 Result = V2(value, value);

	return Result;
}

internal inline v2
V2(v2 P, v2 Q)
{
    v2 Result = V2(Q.x - P.x, Q.y - P.y);

    return Result;
}

internal inline v2
operator+(v2 a, v2 b)
{
	v2 Result;
	Result.x = a.x + b.x;
	Result.y = a.y + b.y;

	return Result;
}

internal inline v2
operator-(v2 v)
{
	v2 Result;
	Result.x = -v.x;
	Result.y = -v.y;

	return Result;
}

internal inline v2
operator-(v2 a, v2 b)
{
	v2 Result;
	Result.x = a.x - b.x;
	Result.y = a.y - b.y;

	return Result;
}

internal inline v2
operator*(f32 f, v2 v)
{
	v2 Result;
	Result.x = f * v.x;
	Result.y = f * v.y;

	return Result;
}

internal inline v2
operator*(v2 v, f32 f)
{
	v2 Result;
	Result = f * v;

	return Result;
}

internal inline v2&
operator+=(v2 &lhs, const v2 &rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}

internal inline v2&
operator-=(v2 &lhs, v2 &rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

internal inline v2&
operator*=(v2 &lhs, f32 f)
{
	lhs.x *= f;
	lhs.y *= f;
	return lhs;
}

internal inline v2&
operator/=(v2 &lhs, f32 f)
{
	lhs.x /= f;
	lhs.y /= f;
	return lhs;
}

internal inline v2 
operator/(const v2 &lhs, f32 f)
{
	v2 Result = {lhs.x / f, lhs.y / f};
	return Result;
}

internal inline f32
Dot(v2 a, v2 b)
{
	f32 Result = a.x * b.x + a.y * b.y;

	return Result;
}

internal inline v2
Reflect(v2 a, v2 n)
{
    v2 Result = a - 2 * Dot(a, n) * n;
    
    return Result;
}

internal inline f32
GetAngle(v2 a, v2 b)
{
    f32 Result;

    if ((a.x == 0.f && a.y == 0.f) ||
        (b.x == 0.f && b.y == 0.f)) {
        Result = 0.f;
    }
    else {
        Result = ACos(Dot(a, b) / Sqrt(Dot(a, a) * Dot(b, b)));
    }

    return Result;
}

internal inline f32
DegreesToRadians(f32 Degrees)
{
	f32 Result = (Degrees * PI) / 180.f;

	return Result;
}

// NOTE: Ax By - Ay Bx
internal inline f32
Curl2D(v2 a, v2 b)
{
	f32 Result = a.x * b.y - a.y * b.x;

	return Result;
}

internal inline v2
Hadamard(v2 a, v2 b)
{
	v2 Result;
	Result.x = a.x * b.x;
	Result.y = a.y * b.y;

	return Result;
}

internal inline v2
Perp(v2 v)
{
	v2 Result;
	Result.x = -v.y;
	Result.y = v.x;

	return Result;
}

internal inline f32
LengthSqr(v2 v)
{
	f32 Result;
	Result = Dot(v, v);

	return Result;
}

internal inline f32
Length(v2 v)
{
	f32 Result;
	Result = Sqrt(Dot(v, v));

	return Result;
}

internal inline v2
Normalize(v2 v)
{
	v2 Result = {};

	f32 L = Length(v);

	if (L > 0) {
		f32 MagInv = 1.f / L;

		Result.x = MagInv * v.x;
		Result.y = MagInv * v.y;
	}

	return Result;
}

internal inline v2
Abs(v2 v)
{
	v2 Result;
	Result.x = Abs(v.x);
	Result.y = Abs(v.y);

	return Result;
}

/* === v4 === */

union v4
{
	struct {
		f32 x;
		f32 y;
		f32 z;
		f32 w;
	};
	struct {
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	};

	f32 E[4];
};

internal inline v4
V4()
{
	v4 Result = {};

	return Result;
}

internal inline v4
V4(f32 x, f32 y, f32 z, f32 w)
{
	v4 Result;
	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;

	return Result;
}

internal inline v4
V4(v2 v)
{
    v4 Result;
    Result.x = v.x;
    Result.y = v.y;
    Result.z = 0.f;
    Result.w = 1.f;

	return Result;
}

internal inline v4
operator+(v4 a, v4 b)
{
	v4 Result;
	Result.x = a.x + b.x;
	Result.y = a.y + b.y;
	Result.z = a.z + b.z;
	Result.w = a.w + b.w;

	return Result;
}

internal inline v4
operator-(v4 v)
{
	v4 Result;
	Result.x = -v.x;
	Result.y = -v.y;
	Result.z = -v.z;
	Result.w = -v.w;

	return Result;
}

internal inline v4
operator-(v4 a, v4 b)
{
	v4 Result;
	Result.x = a.x - b.x;
	Result.y = a.y - b.y;
	Result.z = a.z - b.z;
	Result.w = a.w - b.w;

	return Result;
}

internal inline v4
operator*(f32 f, v4 v)
{
	v4 Result;
	Result.x = f * v.x;
	Result.y = f * v.y;
	Result.z = f * v.z;
	Result.w = f * v.w;

	return Result;
}

internal inline v4
operator*(v4 v, f32 f)
{
	v4 Result;
	Result = f * v;

	return Result;
}

internal inline v4&
operator+=(v4 &lhs, v4 &rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	lhs.w += rhs.w;

	return lhs;
}

internal inline v4
Hadamard(v4 a, v4 b)
{
	v4 Result;
	Result.x = a.x * b.x;
	Result.y = a.y * b.y;
	Result.z = a.z * b.z;
	Result.w = a.w * b.w;

	return Result;
}


/* === RECT === */

struct rect2
{
	v2 Center;
	v2 HalfDim;
};

internal inline rect2
MakeRectHalfDim(v2 Center, v2 HalfDim)
{
	rect2 Result;

	Result.Center = Center;
	Result.HalfDim = HalfDim;

	return Result;
}

internal inline rect2
MakeRectDim(v2 Center, v2 Dim)
{
	rect2 Result = MakeRectHalfDim(Center, 0.5f * Dim);

	return Result;
}

internal inline rect2
MakeRectFromCorners(v2 LowerLeft, v2 UpperRight)
{
	rect2 Result;
	Result.HalfDim = 0.5f * (UpperRight - LowerLeft);
	Result.Center = LowerLeft + Result.HalfDim;

	return Result;
}

internal inline v2
GetMinCorner(rect2 R)
{
    v2 Result = R.Center - R.HalfDim;

    return Result;
}

internal inline v2
GetMaxCorner(rect2 R)
{
    v2 Result = R.Center + R.HalfDim;

    return Result;
}

internal inline rect2
OffsetRect(rect2 R, v2 Offset)
{
    rect2 Result;
    Result.Center = R.Center + Offset;
    Result.HalfDim = R.HalfDim;

    return Result;
}

internal inline rect2
MinkowskiSumRect2(v2 HalfDimA, v2 HalfDimB, v2 Center)
{
	rect2 Result;
	Result.Center = Center;
	Result.HalfDim = HalfDimA + HalfDimB;

	return Result;
}

internal inline rect2
MinkowskiSumRect2(rect2 A, rect2 B, v2 Center)
{
	rect2 Result = MinkowskiSumRect2(A.HalfDim, B.HalfDim, Center);

	return Result;
}

internal inline b32
IsInside(rect2 R, v2 P)
{
	v2 Diff = R.Center - P;
	b32 Result = Abs(Diff.x) < R.HalfDim.x && Abs(Diff.y) < R.HalfDim.y;

	return Result;
}

/* === Matrix 2x2 === */

struct mat2
{
    f32 m[2][2];
};

internal inline mat2
Mat2(v2 x1, v2 x2)
{
    mat2 result;
    result.m[0][0] = x1.x;
    result.m[0][1] = x2.x;
    result.m[1][0] = x1.y;
    result.m[1][1] = x2.y;

    return result;
}

internal inline mat2
operator+(mat2 a, mat2 b)
{
    mat2 result;
    result.m[0][0] = a.m[0][0] + b.m[0][0];
    result.m[0][1] = a.m[0][1] + b.m[0][1];
    result.m[1][0] = a.m[1][0] + b.m[1][0];
    result.m[1][1] = a.m[1][1] + b.m[1][1];
    
    return result;
}

internal inline mat2
operator-(mat2 a, mat2 b)
{
    mat2 result;
    result.m[0][0] = a.m[0][0] - b.m[0][0];
    result.m[0][1] = a.m[0][1] - b.m[0][1];
    result.m[1][0] = a.m[1][0] - b.m[1][0];
    result.m[1][1] = a.m[1][1] - b.m[1][1];
    
    return result;
}

internal inline mat2
operator*(mat2 a, mat2 b)
{
    mat2 result;
    result.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0];
    result.m[0][1] = a.m[0][0] * b.m[1][0] + a.m[0][1] * b.m[1][1];
    result.m[1][0] = a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0];
    result.m[1][1] = a.m[1][0] * b.m[1][0] + a.m[1][1] * b.m[1][1];

    return result;
}

internal inline mat2
Transpose(mat2 a)
{
    mat2 result;
    result.m[0][0] = a.m[0][0];
    result.m[0][1] = a.m[1][0];
    result.m[1][0] = a.m[0][1];
    result.m[1][1] = a.m[1][1];

    return result;
}

internal inline v2
Rotate(v2 v, f32 phi)
{
    v2 result;
    result.x = v.x * Cos(phi) + v.y * Sin(phi);
    result.y = -v.x * Sin(phi) + v.y * Cos(phi);

    return result;
}

/* === Matrix 4x4 === */

union mat4
{
    f32 m[4][4];
	f32 ptr[16];
};

internal inline mat4
Mat4(v4 x1, v4 x2, v4 x3, v4 x4)
{
    mat4 result;
    result.m[0][0] = x1.x;
    result.m[0][1] = x2.x;
    result.m[0][2] = x3.x;
    result.m[0][3] = x4.x;
    result.m[1][0] = x1.y;
    result.m[1][1] = x2.y;
    result.m[1][2] = x3.y;
    result.m[1][3] = x4.y;
    result.m[2][0] = x1.z;
    result.m[2][1] = x2.z;
    result.m[2][2] = x3.z;
    result.m[2][3] = x4.z;
    result.m[3][0] = x1.w;
    result.m[3][1] = x2.w;
    result.m[3][2] = x3.w;
    result.m[3][3] = x4.w;

    return result;
}

internal inline mat4
operator*(mat4 a, mat4 b)
{
    mat4 result;
    result.m[0][0] = 
        a.m[0][0] * b.m[0][0] + 
        a.m[0][1] * b.m[1][0] +
        a.m[0][2] * b.m[2][0] +
        a.m[0][3] * b.m[3][0];

    result.m[0][1] = 
        a.m[0][0] * b.m[0][1] + 
        a.m[0][1] * b.m[1][1] +
        a.m[0][2] * b.m[2][1] +
        a.m[0][3] * b.m[3][1];

    result.m[0][2] = 
        a.m[0][0] * b.m[0][2] + 
        a.m[0][1] * b.m[1][2] +
        a.m[0][2] * b.m[2][2] +
        a.m[0][3] * b.m[3][2];

    result.m[0][3] = 
        a.m[0][0] * b.m[0][3] + 
        a.m[0][1] * b.m[1][3] +
        a.m[0][2] * b.m[2][3] +
        a.m[0][3] * b.m[3][3];

    result.m[1][0] = 
        a.m[1][0] * b.m[0][0] + 
        a.m[1][1] * b.m[1][0] +
        a.m[1][2] * b.m[2][0] +
        a.m[1][3] * b.m[3][0];

    result.m[1][1] = 
        a.m[1][0] * b.m[0][1] + 
        a.m[1][1] * b.m[1][1] +
        a.m[1][2] * b.m[2][1] +
        a.m[1][3] * b.m[3][1];

    result.m[1][2] = 
        a.m[1][0] * b.m[0][2] + 
        a.m[1][1] * b.m[1][2] +
        a.m[1][2] * b.m[2][2] +
        a.m[1][3] * b.m[3][2];

    result.m[1][3] = 
        a.m[1][0] * b.m[0][3] + 
        a.m[1][1] * b.m[1][3] +
        a.m[1][2] * b.m[2][3] +
        a.m[1][3] * b.m[3][3];

    result.m[2][0] = 
        a.m[2][0] * b.m[0][0] + 
        a.m[2][1] * b.m[1][0] +
        a.m[2][2] * b.m[2][0] +
        a.m[2][3] * b.m[3][0];

    result.m[2][1] = 
        a.m[2][0] * b.m[0][1] + 
        a.m[2][1] * b.m[1][1] +
        a.m[2][2] * b.m[2][1] +
        a.m[2][3] * b.m[3][1];

    result.m[2][2] = 
        a.m[2][0] * b.m[0][2] + 
        a.m[2][1] * b.m[1][2] +
        a.m[2][2] * b.m[2][2] +
        a.m[2][3] * b.m[3][2];

    result.m[2][3] = 
        a.m[2][0] * b.m[0][3] + 
        a.m[2][1] * b.m[1][3] +
        a.m[2][2] * b.m[2][3] +
        a.m[2][3] * b.m[3][3];

    result.m[3][0] = 
        a.m[3][0] * b.m[0][0] + 
        a.m[3][1] * b.m[1][0] +
        a.m[3][2] * b.m[2][0] +
        a.m[3][3] * b.m[3][0];

    result.m[3][1] = 
        a.m[3][0] * b.m[0][1] + 
        a.m[3][1] * b.m[1][1] +
        a.m[3][2] * b.m[2][1] +
        a.m[3][3] * b.m[3][1];

    result.m[3][2] = 
        a.m[3][0] * b.m[0][2] + 
        a.m[3][1] * b.m[1][2] +
        a.m[3][2] * b.m[2][2] +
        a.m[3][3] * b.m[3][2];

    result.m[3][3] = 
        a.m[3][0] * b.m[0][3] + 
        a.m[3][1] * b.m[1][3] +
        a.m[3][2] * b.m[2][3] +
        a.m[3][3] * b.m[3][3];

    return result;
}

/* === Utility === */

struct line_segment
{
    v2 P;
    v2 Q;
};

internal inline line_segment
MakeLineSegment(v2 P, v2 Q)
{
    line_segment Result;
    Result.P = P;
    Result.Q = Q;

    return Result;
}

internal inline mat4 
CreateViewProjection(v2 WorldPosition, v2 Dim)
{
    v2 Scale = V2(2.f / Dim.x, 2.f / Dim.y);
    mat4 Result = 
        Mat4(
            V4(Scale.x,     0.f, 0.f, -Scale.x * WorldPosition.x), 
            V4(    0.f, Scale.y, 0.f, -Scale.y * WorldPosition.y), 
            V4(    0.f,     0.f, 1.f,                        0.f), 
            V4(    0.f,     0.f, 0.f,                        1.f)
        );

    return Result;
}

internal inline b32 
TestLineSegments(line_segment A, line_segment B, v2 *Hit)
{
    b32 Result = false;
    v2 VecA = V2(A.P, A.Q);
    v2 VecB = V2(B.P, B.Q);
    f32 CurlAB = Curl2D(VecA, VecB);
    if(CurlAB != 0.f) {
        v2 Rel = B.P - A.P;
        f32 CurlAD = Curl2D(VecA, Rel);
        f32 t = -(CurlAD / CurlAB);
        if(t >= 0.f && t <= 1.f) {
            f32 s;
            f32 eps = 0.001f;
            if(Abs(VecA.x) >= eps) {
                s = (Rel.x + t * VecB.x) / VecA.x;
            }
            else if(Abs(VecA.y) >= eps) {
                s = (Rel.y + t * VecB.y) / VecA.y;
            }
            else {
                s = -1.f;
            }

            if(s >= 0.f && s <= 1.f) {
                *Hit = B.P + t * VecB;
                Result = true;
            }
        }
    }

    return Result;
}

struct ray2
{
	v2 Start;
	v2 Dir;
};

internal inline ray2 Ray2(const v2 &Start, const v2 &Dir)
{
	ray2 Result = {Start, Dir};
	return Result;
}

internal inline ray2 Ray2FromPoints(const v2 &A, const v2 &B)
{
	ray2 Result = Ray2(A, Normalize(B - A));
	return Result;
}

// NOTE: A, B are starting points for the ray, u and v are normalized directions: (A, u), (B, v)
internal b32 RayIntersect2(v2 A, v2 u, v2 B, v2 v, f32 *tParam, f32 *sParam)
{
	b32 Result;
	f32 uv, uv2, t, s;
	v2 w, k, l;

	w = B - A;
	uv = Dot(u, v);
	uv2 = uv * uv;
	if(uv2 != 1.f) {
		k = (u - uv * v) / (1.f - uv2);
		t = Dot(w, k);
		if(tParam) {
			*tParam = t;
		}
		if(sParam) {
			l = (v - uv * u) / (uv2 - 1.f);
			s = Dot(w, l);
			*sParam = s;
		}
		Result = true;
	}
	else {
		// NOTE: Parallel, default to no intersection
		// TODO: Could test for coinciding rays
		Result = false;
	}
	return Result;
}

internal b32 RayIntersect2(const ray2 &RayA, const ray2 &RayB, f32 *tParam, f32 *sParam)
{
	return RayIntersect2(RayA.Start, RayA.Dir, RayB.Start, RayB.Dir, tParam, sParam);
}

internal inline b32 
Ray2IntersectsSegment(const ray2 &Ray, const v2 &SegmentP, const v2 &SegmentQ, f32 *sParam = 0)
{
	b32 Result;
	v2 SegVec;
	f32 SegLen, s;
	ray2 RaySegment;

	SegVec = SegmentQ - SegmentP;
	SegLen = Length(SegVec);
	
	RaySegment = Ray2(SegmentP, SegVec / SegLen);
	
	if(RayIntersect2(Ray, RaySegment, 0, &s)) {
		Result = (s >= 0.f && s < SegLen);
		if(sParam)
			*sParam = s / SegLen;
	}
	else {
		Result = false;
	}

	return Result;
}

union basis2d
{
	struct {
		v2 E1;
		v2 E2;
	};
	v2 E[2];
	f32 *Data;
};

internal inline basis2d CreateBasisFrom(v2 E1, v2 E2)
{
	basis2d Result;
	Result.E1 = E1;
	Result.E2 = E2;

	return Result;
}

internal inline basis2d ScaleBasis(basis2d Basis, f32 ScaleX, f32 ScaleY)
{
	basis2d Result;
	Result.E1 = ScaleX * Basis.E1;
	Result.E2 = ScaleY * Basis.E2;

	return Result;
}

internal inline basis2d CanonicalBasis()
{
	basis2d Result;
	Result.E1 = V2(1.f, 0.f);
	Result.E2 = V2(0.f, 1.f);

	return Result;
}

internal inline basis2d
operator*(basis2d B, f32 Scalar)
{
	basis2d Result;
	Result.E1 = Scalar * B.E1;
	Result.E2 = Scalar * B.E2;

	return Result;
}

internal inline basis2d
operator*(f32 Scalar, basis2d B)
{
	basis2d Result;
	Result.E1 = Scalar * B.E1;
	Result.E2 = Scalar * B.E2;

	return Result;
}

internal inline basis2d&
operator*=(basis2d& B, f32 Scalar)
{
	B.E1 *= Scalar;
	B.E2 *= Scalar;
	return B;
}

internal inline v2 Transform(v2 A, basis2d Basis)
{
	v2 Result;
	Result.x = A.x * Basis.E1.x + A.y * Basis.E2.x;
	Result.y = A.x * Basis.E1.y + A.y * Basis.E2.y;

	return Result;
}



/* --- U T I L I T Y --- */

internal inline s8 SafeTruncateS8(f32 Value)
{
	s8 Result = (s8)Clamp(Value, (f32)S8_MIN, (f32)S8_MAX);
	return Result;
}

internal inline s16 SafeTruncateS16(f32 Value)
{
	s16 Result = (s16)Clamp(Value, (f32)S16_MIN, (f32)S16_MAX);
	return Result;
}

internal inline s32 SafeTruncateS32(f32 Value)
{
	// NOTE: S32_MIN is a power of 2
	s32 Result = (s32)Clamp(Value, (f32)S32_MIN, (f32)S32_MAX - 65);
	return Result;
}

internal inline u8 SafeTruncateU8(f32 Value)
{
	u8 Result = (u8)Clamp(Value, (f32)U8_MIN, (f32)U8_MAX);
	return Result;
}

internal inline u16 SafeTruncateU16(f32 Value)
{
	u16 Result = (u16)Clamp(Value, (f32)U16_MIN, (f32)U16_MAX);
	return Result;
}

internal inline u32 SafeTruncateU32(f32 Value)
{
	// NOTE: Values from (U32_MAX - 127) to U32_MAX will be converted to U32_MAX + 1 because we only have 24 bit of precision inside the mantissa
	u32 Result = (u32)Clamp(Value, (f32)U32_MIN, (f32)(U32_MAX - 128));
	return Result;
}

internal inline s8 SafeRoundS8(f32 Value)
{
	s8 Result = SafeTruncateS8((Value < 0) ? Value - 0.5f : Value + 0.5f);
	return Result;
}

internal inline s16 SafeRoundS16(f32 Value)
{
	s16 Result = SafeTruncateS16((Value < 0) ? Value - 0.5f : Value + 0.5f);
	return Result;
}

internal inline s32 SafeRoundS32(f32 Value)
{
	s32 Result = SafeTruncateS32((Value < 0) ? Value - 0.5f : Value + 0.5f);
	return Result;
}

internal inline u8 SafeRoundU8(f32 Value)
{
	u8 Result;
	if(Value <= 0.f) {
		Result = 0;
	}
	else {
		Result = SafeTruncateU8(Value + 0.5f);
	}
	return Result;
}

internal inline u16 SafeRoundU16(f32 Value)
{
	u16 Result;
	if(Value <= 0.f) {
		Result = 0;
	}
	else {
		Result = SafeTruncateU16(Value + 0.5f);
	}
	return Result;
}

internal inline u32 SafeRoundU32(f32 Value)
{
	u32 Result;
	if(Value <= 0.f) {
		Result = 0;
	}
	else {
		Result = SafeTruncateU32(Value + 0.5f);
	}
	return Result;
}

#endif
