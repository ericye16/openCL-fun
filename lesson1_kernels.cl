#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
__kernel void hello(__global int * out, __global int * in)
{
    size_t tid = get_global_id(0);
    out[tid] = in[tid] + 13;
}
