float4 quat(in float3 axis, in float angle)
{
    return float4(axis * sin(angle/2.0), cos(angle/2.0));
}

float4 qmul(in float4 q1, in float4 q2)
{
    return float4(q1.xyz*q2.w + q2.xyz*q1.w + cross(q1.xyz, q2.xyz), q1.w*q2.w - dot(q1.xyz,q2.xyz));
}

float4 qinv(in float4 q)
{
    return float4(-q.xyz, q.w);
}

float3 qrot(in float3 p, in float4 q)
{
    return qmul(qmul(q, float4(p, 0.0)), qinv(q)).xyz;
}
