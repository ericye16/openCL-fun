__kernel void vecadd(
__global float3 * A,
__global float3 * B,
__global float * C)
{
int idx = get_global_id(0);
C[idx] = dot(A[idx], B[idx]);
}