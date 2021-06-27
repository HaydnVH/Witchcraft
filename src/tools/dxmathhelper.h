/* dxmathhelper.h
 * DirectXMath Helper
 * by Haydn V. Harach
 * Created October 2019
 *
 * This file provides a handful of types and functions which make using
 * the DirectX math library more convenient.  Using inline functions
 * and defines, any performance impact is minimized.
 */
#ifndef HVH_TOOLKIT_DXMATHHELPER_H
#define HVH_TOOLKIT_DXMATHHELPER_H

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <cstdint>
#include <cmath>
#include <algorithm>

/******************************************************************************
 * Packed floats
 *****************************************************************************/

// Two floats packed together.
typedef DirectX::XMFLOAT2 float2;
// Three floats packed together.
typedef DirectX::XMFLOAT3 float3;
// Four floats packed together.
typedef DirectX::XMFLOAT4 float4;

/******************************************************************************
 * Half-float
 *****************************************************************************/

// 16-bit floating point value (half precision).
typedef DirectX::PackedVector::HALF half;
// Convert float to half.
#define ftoh(f) DirectX::PackedVector::XMConvertFloatToHalf(f)
// Convert half to float.
#define htof(h) DirectX::PackedVector::XMConvertHalfToFloat(h)

/******************************************************************************
 * Vector
 *****************************************************************************/

// SIMD vector type storing 4 floats.
typedef DirectX::XMVECTOR vec4;
// Create a vector using four floats.
#define vec4_set(x,y,z,w) DirectX::XMVectorSet(x,y,z,w)
// Create a vector using two float2s.
inline vec4 vec4_make(float2 xy, float2 zw) { return vec4_set(xy.x, xy.y, zw.x, zw.y); }
// Create a vector using a float3 and a float.
inline vec4 vec4_make(float3 xyz, float w = 0.0f) { return vec4_set(xyz.x, xyz.y, xyz.z, w); }
// Create a vector using a float4.
inline vec4 vec4_make(float4 xyzw) { return vec4_set(xyzw.x, xyzw.y, xyzw.z, xyzw.w); }
// Get the x (1st) component of a vector.
#define vec4_x(v) DirectX::XMVectorGetX(v)
// Get the y (2nd) component of a vector.
#define vec4_y(v) DirectX::XMVectorGetY(v)
// Get the z (3rd) component of a vector.
#define vec4_z(v) DirectX::XMVectorGetZ(v)
// Get the w (4th) component of a vector.
#define vec4_w(v) DirectX::XMVectorGetW(v)
// Get the nth component of a vector.
#define vec4_n(v,n) DirectX::XMVectorGetByIndex(v,n)
// Get the x, y, and z components of a vector as a float3.
#define vec4_xyz(v) float3(vec4_x(v), vec4_y(v), vec4_z(v))
// Get the x, y, z, and w components of a vector as a float4.
#define vec4_xyzw(v) float4(vec4_x(v), vec4_y(v), vec4_z(v), vec4_w(v))
// Rotate a vector according to a quaternion.
#define vec4_rotate(v,q) DirectX::XMVector3Rotate(v,q)
// Get the cross product between two vectors.
#define vec4_cross(l,r) DirectX::XMVector3Cross(l,r)
// Linearly interpolate between two vectors.
#define vec4_lerp(l,r,t) DirectX::XMVectorLerp(l,r,t)

inline vec4 operator + (vec4 l, vec4 r) { return DirectX::XMVectorAdd(l, r); }
inline vec4 operator - (vec4 l, vec4 r) { return DirectX::XMVectorSubtract(l, r); }
inline vec4 operator * (vec4 l, vec4 r) { return DirectX::XMVectorMultiply(l, r); }
inline vec4 operator / (vec4 l, vec4 r) { return DirectX::XMVectorDivide(l, r); }
inline vec4 operator * (vec4 l, float r) { return DirectX::XMVectorMultiply(l, vec4_set(r,r, r,r)); }
inline vec4 operator / (vec4 l, float r) { return DirectX::XMVectorDivide(l, vec4_set(r,r,r,r)); }

inline vec4 operator += (vec4& l, vec4 r) { l = DirectX::XMVectorAdd(l, r); return l; }
inline vec4 operator -= (vec4& l, vec4 r) { l = DirectX::XMVectorSubtract(l, r); return l; }
inline vec4 operator *= (vec4& l, vec4 r) { l = DirectX::XMVectorMultiply(l, r); return l; }
inline vec4 operator /= (vec4& l, vec4 r) { l = DirectX::XMVectorDivide(l, r); return l; }
inline vec4 operator *= (vec4& l, float r) { l = DirectX::XMVectorMultiply(l, vec4_set(r,r,r,r)); return l; }
inline vec4 operator /= (vec4& l, float r) { l = DirectX::XMVectorDivide(l, vec4_set(r,r,r,r)); return l; }

inline vec4 operator - (vec4 v) { return DirectX::XMVectorNegate(v); }

/******************************************************************************
 * Quaternion
 *****************************************************************************/

// SIMD vector type representing a quaternion.
typedef DirectX::XMVECTOR quat;
// Create a quaternion using four floats.
#define quat_set(x,y,z,w) DirectX::XMVectorSet(x,y,z,w)
// Get the x (1st) component of a quaternion.
#define quat_x(q) DirectX::XMVectorGetX(q)
// Get the y (2nd) component of a quaternion.
#define quat_y(q) DirectX::XMVectorGetY(q)
// Get the z (3rd) component of a quaternion.
#define quat_z(q) DirectX::XMVectorGetZ(q)
// Get th w (4th) component of a quaternion.
#define quat_w(q) DirectX::XMVectorGetW(q)
// Multiply two quaternions together, concatenating their rotations.
#define quat_mul(l,r) DirectX::XMQuaternionMultiply(l,r)
// Spherical linear interpolation between two quaternions.
#define quat_slerp(l,r,t) DirectX::XMQuaternionSlerp(l,r,t)
// Create a quaternion from a set of euler angles (pitch, yaw, tilt)
#define quat_euler(x,y,z) DirectX::XMQuaternionRotationRollPitchYaw(x,y,z)
// Create a quaternion from an axis vector and an angle.
#define quat_axis(v,a) DirectX::XMQuaternionRotationAxis(v,a)
// Create an identity quaternion, representing no rotation.
#define quat_identity() DirectX::XMQuaternionIdentity()
// Get the conjugate (inverse) of a quaternion.
#define quat_conjugate(q) DirectX::XMQuaternionConjugate(q)

/******************************************************************************
 * Matrix
 *****************************************************************************/

// 4x4 matrix using made of four SIMD vectors.
typedef DirectX::XMMATRIX mat4;
// Create an identity matrix, representing no transformation.
#define mat4_identity() DirectX::XMMatrixIdentity()
// Create a matrix from a series of 16 floats.
#define mat4_set(m11,m12,m13,m14,m21,m22,m23,m24,m31,m32,m33,m34,m41,m42,m43,m44) DirectX::XMMatrixSet(m11,m12,m13,m14,m21,m22,m23,m24,m31,m32,m33,m34,m41,m42,m43,m44)
// Multiply two matrices together.
#define mat4_mul(l,r) DirectX::XMMatrixMultiply(l,r)
// Create a matrix which represents a translation.
#define mat4_translation(v) DirectX::XMMatrixTranslationFromVector(v)
// Create a matrix which represents a rotation.
#define mat4_rotation(q) DirectX::XMMatrixRotationQuaternion(q)
// Create a matrix which represents a scale.
#define mat4_scale(v) DirectX::XMMatrixScalingFromVector(v)
// Create a perspective projection matrix.
#define mat4_perspective(fov,aspect,near,far) DirectX::XMMatrixPerspectiveFovLH(fov,aspect,near,far)
// Create a perspective projection matrix with reversed depth.
#define mat4_perspective_reversed(fov,aspect,near,far) DirectX::XMMatrixPerspectiveFovLH(fov,aspect,far,near)
// Calculate the inverse of a matrix.
#define mat4_invert(m) DirectX::XMMatrixInverse(nullptr,m)
// Decompose a matrix into scale, rotation, and translation.
#define mat4_decompose(m,s,r,t) DirectX::XMMatrixDecompose(&s,&r,&t,m)

/******************************************************************************
 * Normalized real numbers
 *****************************************************************************/

// An unsigned 8-bit integer used to represent a value between 0 and 1.
typedef uint8_t unorm8;
inline unorm8 unorm8_set(float f) { return (unorm8)(std::max(0.0f, std::min((float)UCHAR_MAX, f * (float)UCHAR_MAX))); }
inline float unorm8_get(unorm8 n) { return (float)n / (float)UCHAR_MAX; }

// A signed 8-bit integer used to represent a value between -1 and 1.
typedef int8_t norm8;
inline norm8 norm8_set(float f) { return (norm8)(std::max((float)SCHAR_MIN, std::min((float)SCHAR_MAX, f * (float)SCHAR_MAX))); }
inline float norm8_get(norm8 n) { return (float)n / (float)SCHAR_MAX; }

// An unsigned 16-bit integer used to represent a value between 0 and 1.
typedef unsigned short unorm16;
inline unorm16 unorm16_set(float f) { return (unorm16)(std::max(0.0f, std::min((float)USHRT_MAX, f * (float)USHRT_MAX))); }
inline float unorm16_get(unorm16 n) { return (float)n / (float)USHRT_MAX; }

// A signed 16-bit integer used to represent a value between -1 and 1.
typedef short norm16;
inline norm16 norm16_set(float f) { return (norm16)(std::max((float)SHRT_MIN, std::min((float)SHRT_MAX, f * (float)SHRT_MAX))); }
inline float norm16_get(norm16 n) { return (float)n / (float)SHRT_MAX; }

/******************************************************************************
 * Constants
 *****************************************************************************/

constexpr const float2 FLOAT2_ZERO = { 0.0f, 0.0f };
constexpr const float2 FLOAT2_ONE = { 1.0f, 1.0f };
constexpr const float2 FLOAT2_RIGHT = { 1.0f, 0.0f };
constexpr const float2 FLOAT2_LEFT = { -1.0f, 0.0f };
constexpr const float2 FLOAT2_DOWN = { 0.0f, 1.0f };
constexpr const float2 FLOAT2_UP = { 0.0f, -1.0f };

constexpr const float3 FLOAT3_ZERO = { 0.0f, 0.0f, 0.0f };
constexpr const float3 FLOAT3_ONE = { 1.0f, 1.0f, 1.0f };
constexpr const float3 FLOAT3_RIGHT = { 1.0f, 0.0f, 0.0f };
constexpr const float3 FLOAT3_LEFT = { -1.0f, 0.0f, 0.0f };
constexpr const float3 FLOAT3_UP = { 0.0f, 1.0f, 0.0f };
constexpr const float3 FLOAT3_DOWN = { 0.0f, -1.0f, 0.0f };
constexpr const float3 FLOAT3_FORWARD = { 0.0f, 0.0f, 1.0f };
constexpr const float3 FLOAT3_BACK = { 0.0f, 0.0f, -1.0f };

constexpr const float4 FLOAT4_ZERO = { 0.0f, 0.0f, 0.0f, 0.0f };
constexpr const float4 FLOAT4_ONE = { 1.0f, 1.0f, 1.0f, 1.0f };

constexpr const quat QUAT_IDENTITY = { 0.0f, 0.0f, 0.0f, 1.0f };

constexpr const vec4 VECTOR_ZERO = { 0.0f, 0.0f, 0.0f, 0.0f };
constexpr const vec4 VECTOR_ONE = { 1.0f, 1.0f, 1.0f, 1.0f };
constexpr const vec4 VECTOR_RIGHT = { 1.0f, 0.0f, 0.0f, 0.0f };
constexpr const vec4 VECTOR_LEFT = { -1.0f, 0.0f, 0.0f, 0.0f };
constexpr const vec4 VECTOR_UP = { 0.0f, 1.0f, 0.0, 0.0f };
constexpr const vec4 VECTOR_DOWN = { 0.0f, -1.0f, 0.0f, 0.0f };
constexpr const vec4 VECTOR_FORWARD = { 0.0f, 0.0f, 1.0f, 0.0f };
constexpr const vec4 VECTOR_BACK = { 0.0f, 0.0f, -1.0f, 0.0f };

constexpr const float PI = DirectX::XM_PI;
constexpr const float TAU = PI*2;
constexpr const float DEG2RAD = PI/180.0f;
constexpr const float RAD2DEG = 180.0f/PI;


#endif // HVH_TOOLKIT_DXMATHHELPER_H