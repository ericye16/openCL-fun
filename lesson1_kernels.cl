#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
__constant int hw[]= {1,2,3,4,5,6,7,8,9,10,11,12,13};
__kernel void hello(__global int * out, __global int * in)
{
    size_t tid = get_global_id(0);
    out[tid] = (int) tid;
}
