import Shared.ShaderData;

// ---- Bit-packed visibility set ----- packed 1 bit per draw / meshlet -----

bool VisBitTest(uint64_t bufAddr, uint idx)
{
    uint* bits = (uint*)bufAddr;
    uint  word = bits[idx >> 5u];
    return (word & (1u << (idx & 31u))) != 0u;
}

void VisBitSet(uint64_t bufAddr, uint idx)
{
    // Slang's BDA pointers don't expose atomicOr() directly; cast to a generic
    // pointer of the right kind so the SPIRV emitter picks the atomic op.
    uint* bits = (uint*)bufAddr;
    uint  mask = 1u << (idx & 31u);
    InterlockedOr(bits[idx >> 5u], mask);
}

// ---- View-space frustum + sphere projection (identical math to GLSL) -----

bool sphereInsideFrustum(float3 c, float r, float4 frustum, float zNear, float zFar)
{
    bool visible = true;
    visible = visible && c.z * frustum.y - abs(c.x) * frustum.x > -r;
    visible = visible && c.z * frustum.w - abs(c.y) * frustum.z > -r;
    visible = visible && c.z + r > zNear && c.z - r < zFar;
    return visible;
}

bool projectSphere(float3 c, float r, float znear, float P00, float P11, out float4 aabb)
{
    aabb = float4(0, 0, 0, 0);
    if (c.z < r + znear)
        return false;

    float3 cr   = c * r;
    float  czr2 = c.z * c.z - r * r;

    float vx   = sqrt(c.x * c.x + czr2);
    float minx = (vx * c.x - cr.z) / (vx * c.z + cr.x);
    float maxx = (vx * c.x + cr.z) / (vx * c.z - cr.x);

    float vy   = sqrt(c.y * c.y + czr2);
    float miny = (vy * c.y - cr.z) / (vy * c.z + cr.y);
    float maxy = (vy * c.y + cr.z) / (vy * c.z - cr.y);

    aabb = float4(minx * P00, miny * P11, maxx * P00, maxy * P11);
    aabb = aabb.xwzy * float4(0.5, -0.5, 0.5, -0.5) + float4(0.5);
    return true;
}

// Quaternion rotation, q.xyz is axis * sin(t/2), q.w is cos(t/2).
float3 RotateByQuaternion(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}