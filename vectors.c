

#include <stdio.h>
#include <stdlib.h>
#include<stdbool.h>

// OpenCL includes
#include <CL/cl.h>

#define FLUID_MASS 1.0f //highly arbitrary

const int xSize = 32;
const int ySize = 32;
const int zSize = 32;

const cl_float3 gravity = {0.f, -9.8f, 0.f};

int lin(int x, int y, int z); 

void initialize(cl_float3 * position, cl_float3 * velocity, cl_float * mass);


int main() {
    fprintf(stderr, "asdf\n");
    // Elements in each array

    int elements;

    elements = xSize * ySize * zSize;
    // OpenCL kernel
    // This code executes on the OpenCL host
    FILE * kernelFile = NULL;
    kernelFile = fopen("vectors_kernel.cl", "r");
    fseek(kernelFile, 0L, SEEK_END);
    size_t programSize;
    programSize = ftell(kernelFile);
    rewind(kernelFile);
    fprintf(stderr,"Kernel file is %i bytes\n", programSize);

    char * kernelCode;
    kernelCode = (char*)malloc(programSize + 1);
    kernelCode[programSize] = '\0';
    fread(kernelCode, sizeof(char), programSize, kernelFile);
    fclose(kernelFile);
    
    fprintf(stderr,"Completed reading kernel file.\n");

    
    // Host data
    
    cl_float3 *velocity = NULL;
    cl_float3 *position = NULL;
    cl_float3 *velocity_final = NULL;
    cl_float3 *position_final = NULL;
    cl_float *mass = NULL;
    //cl_float * result_dot = NULL;
    
    // Compute the size of the data 
    size_t datasize_vel = sizeof(cl_float3) * elements;
    size_t datasize_pos = sizeof(cl_float3) * elements;
    size_t datasize_pressure = sizeof(cl_float) * elements;
    size_t datasize_result = sizeof(float) * elements;

    // Allocate space for input/output data
    
    //initialize our data
    position = (cl_float3*)malloc(datasize_pos);
    velocity = (cl_float3*)malloc(datasize_vel);
    mass = (cl_float*)malloc(datasize_pressure);
    //result_dot = (cl_float*)malloc(datasize_result);
    
    // Initialize the input data
    initialize(position, velocity, mass);
    fprintf(stderr, "Initialized position and velocity.\n");

    // Use this to check the output of each API call`
    cl_int status;  
     
    //-----------------------------------------------------
    // STEP 1: Discover and initialize the platforms
    //-----------------------------------------------------
    
    cl_uint numPlatforms = 0;
    cl_platform_id *platforms = NULL;
    
    // Use clGetPlatformIDs() to retrieve the number of 
    // platforms
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
 
    // Allocate enough space for each platform
    platforms =   
        (cl_platform_id*)malloc(
            numPlatforms*sizeof(cl_platform_id));
 
    // Fill in platforms with clGetPlatformIDs()
    status = clGetPlatformIDs(numPlatforms, platforms, 
                NULL);

    //-----------------------------------------------------
    // STEP 2: Discover and initialize the devices
    //----------------------------------------------------- 
    
    cl_uint numDevices = 0;
    cl_device_id *devices = NULL;

    // Use clGetDeviceIDs() to retrieve the number of 
    // devices present
    status = clGetDeviceIDs(
        platforms[0], 
        CL_DEVICE_TYPE_ALL, 
        0, 
        NULL, 
        &numDevices);

    // Allocate enough space for each device
    devices = 
        (cl_device_id*)malloc(
            numDevices*sizeof(cl_device_id));

    // Fill in devices with clGetDeviceIDs()
    status = clGetDeviceIDs(
        platforms[0], 
        CL_DEVICE_TYPE_ALL,        
        numDevices, 
        devices, 
        NULL);

    //-----------------------------------------------------
    // STEP 3: Create a context
    //----------------------------------------------------- 
    
    cl_context context = NULL;

    // Create a context using clCreateContext() and 
    // associate it with the devices
    context = clCreateContext(
        NULL, 
        numDevices, 
        devices, 
        NULL, 
        NULL, 
        &status);

    //-----------------------------------------------------
    // STEP 4: Create a command queue
    //----------------------------------------------------- 
    
    cl_command_queue cmdQueue;

    // Create a command queue using clCreateCommandQueue(),
    // and associate it with the device you want to execute 
    // on
    cmdQueue = clCreateCommandQueue(
        context, 
        devices[0], 
        0, 
        &status);
    fprintf(stderr, "Context and command queues created.\n");

    //-----------------------------------------------------
    // STEP 5: Create device buffers
    //----------------------------------------------------- 
    
    /*cl_mem bufferA;  // Input array on the device
    cl_mem bufferB;  // Input array on the device
    cl_mem bufferC;  // Output array on the device
    */
    
    cl_mem bufferVel;
    cl_mem bufferPos;
    cl_mem bufferResult;
    cl_mem bufferVelFinal;
    cl_mem bufferPosFinal;

    // Use clCreateBuffer() to create a buffer object (d_A) 
    // that will contain the data from the host array A
    /*bufferA = clCreateBuffer(
        context, 
        CL_MEM_READ_ONLY,                         
        datasize, 
        NULL, 
        &status);

    // Use clCreateBuffer() to create a buffer object (d_B)
    // that will contain the data from the host array B
    bufferB = clCreateBuffer(
        context, 
        CL_MEM_READ_ONLY,                         
        datasize, 
        NULL, 
        &status);

    // Use clCreateBuffer() to create a buffer object (d_C) 
    // with enough space to hold the output data
    bufferC = clCreateBuffer(
        context, 
        CL_MEM_WRITE_ONLY,                 
        datasize, 
        NULL, 
        &status);*/
    
    bufferVel = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        datasize_vel,
        NULL,
        &status);
    bufferPos = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        datasize_pos,
        NULL,
        &status);
    bufferResult = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        datasize_result,
        NULL,
        &status);
    
    
    fprintf(stderr, "Buffers created\n");
    
        
    //-----------------------------------------------------
    // STEP 6: Write host data to device buffers
    //----------------------------------------------------- 
    
    // Use clEnqueueWriteBuffer() to write input array A to
    // the device buffer bufferA
    /*status = clEnqueueWriteBuffer(
        cmdQueue, 
        bufferA, 
        CL_FALSE, 
        0, 
        datasize,                         
        A, 
        0, 
        NULL, 
        NULL);
    
    // Use clEnqueueWriteBuffer() to write input array B to 
    // the device buffer bufferB
    status = clEnqueueWriteBuffer(
        cmdQueue, 
        bufferB, 
        CL_FALSE, 
        0, 
        datasize,                                  
        B, 
        0, 
        NULL, 
        NULL);*/
    status = clEnqueueWriteBuffer(
        cmdQueue,
        bufferVel,
        CL_FALSE,
        0,
        datasize_vel,
        velocity,
        0,
        NULL,
        NULL);
    if (status != CL_SUCCESS) {
        fprintf(stderr, "Could not write buffer");
    }
    
    status = clEnqueueWriteBuffer(
        cmdQueue,
        bufferPos,
        CL_FALSE,
        0,
        datasize_pos,
        position,
        0,
        NULL,
        NULL);
    if (status != CL_SUCCESS) {
        fprintf(stderr, "Could not write buffer");
    }
    
    
    fprintf(stderr, "Buffers written\n");

    //-----------------------------------------------------
    // STEP 7: Create and compile the program
    //----------------------------------------------------- 
     
    // Create a program using clCreateProgramWithSource()
    cl_program program = clCreateProgramWithSource(
        context, 
        1, 
        (const char**)&kernelCode,                                 
        NULL, 
        &status);
    free(kernelCode);
    
    fprintf(stderr, "Kernel code created and freed\n");

    // Build (compile) the program for the devices with
    // clBuildProgram()
    status = clBuildProgram(
        program, 
        numDevices, 
        devices, 
        NULL, 
        NULL, 
        NULL);
    if (status != CL_SUCCESS) {
        fprintf(stderr, "Kernel failed to build");
        return -1;
    }
    //-----------------------------------------------------
    // STEP 8: Create the kernel
    //----------------------------------------------------- 

    cl_kernel kernel = NULL;

    // Use clCreateKernel() to create a kernel from the 
    // vector addition function (named "vecadd")
    kernel = clCreateKernel(program, "fluids", &status);
    fprintf(stderr, "Kernel created\n");

    //-----------------------------------------------------
    // STEP 9: Set the kernel arguments
    //----------------------------------------------------- 
    
    // Associate the input and output buffers with the 
    // kernel 
    // using clSetKernelArg()
    status  = clSetKernelArg(
        kernel, 
        0, 
        sizeof(cl_mem), 
        &bufferVel);
    status |= clSetKernelArg(
        kernel, 
        1, 
        sizeof(cl_mem), 
        &bufferPos);
    status |= clSetKernelArg(
        kernel, 
        2, 
        sizeof(cl_mem), 
        &bufferResult);
    
    fprintf(stderr, "Kernel arguments set\n");

    //-----------------------------------------------------
    // STEP 10: Configure the work-item structure
    //----------------------------------------------------- 
    
    // Define an index space (global work size) of work 
    // items for 
    // execution. A workgroup size (local work size) is not 
    // required, 
    // but can be used.
    cl_uint numDims = 3;
    size_t globalWorkSize[numDims];    
    // There are 'elements' work-items 
    globalWorkSize[0] = xSize;
    globalWorkSize[1] = ySize;
    globalWorkSize[2] = zSize;
    size_t localWorkSize[numDims];
    localWorkSize[0] = 8;
    localWorkSize[1] = 8;
    localWorkSize[2] = 8;
    
    fprintf(stderr, "Local and global workgroup sizes set\n");

    //-----------------------------------------------------
    // STEP 11: Enqueue the kernel for execution
    //----------------------------------------------------- 
    
    // Execute the kernel by using 
    // clEnqueueNDRangeKernel().
    // 'globalWorkSize' is the 1D dimension of the 
    // work-items
    status = clEnqueueNDRangeKernel(
        cmdQueue, 
        kernel, 
        numDims, 
        NULL, 
        globalWorkSize, 
        localWorkSize, 
        0, 
        NULL, 
        NULL);
    if (status != CL_SUCCESS) {
        fprintf(stderr, "Kernels not launched.");
    }
    fprintf(stderr, "Kernel launched and run.\n");

    //-----------------------------------------------------
    // STEP 12: Read the output buffer back to the host
    //----------------------------------------------------- 
    
    // Use clEnqueueReadBuffer() to read the OpenCL output  
    // buffer (bufferC) 
    // to the host output array (C)
    /*clEnqueueReadBuffer(
        cmdQueue, 
        bufferResult, 
        CL_TRUE, 
        0, 
        datasize_result, 
        result_dot, 
        0, 
        NULL, 
        NULL);*/

    // Verify the output
    bool result = true;
    /*for(int i = 0; i < elements; i++) {
        printf("%f ",result_dot[i]);
        if(C[i] != i+i) {
            result = false;
            break;
        }
    }*/
    /*if(result) {
        printf("Output is correct\n");
    } else {
        printf("Output is incorrect\n");
    }*/

    //-----------------------------------------------------
    // STEP 13: Release OpenCL resources
    //----------------------------------------------------- 
    
    // Free OpenCL resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(cmdQueue);
    /*clReleaseMemObject(bufferA);
    clReleaseMemObject(bufferB);
    clReleaseMemObject(bufferC);*/
    clReleaseContext(context);
    fprintf(stderr, "Released kernel, program and queue\n");

    // Free host resources
    free(velocity);
    fprintf(stderr, "Released velocity\n");
    free(position);
    fprintf(stderr, "Released position\n");
    free(mass);
    fprintf(stderr, "Released mass\n");
    /*free(position_final);
    free(velocity_final);*/

    free(platforms);
    free(devices);
    fprintf(stderr, "End of program.\n");
    
    
    return 0;
}

int lin(int x, int y, int z)
{
    return z * xSize * ySize + xSize * y + x;
}

void initialize(cl_float3 * position, cl_float3 * velocity, cl_float * mass) {
    for(int x = 0; x < xSize; x++) {
        for (int y = 0; y < ySize; y++) {
            for (int z = 0; z < zSize; z++) {
                position[lin(x,y,z)].x = x;
                position[lin(x,y,z)].y = y;
                position[lin(x,y,z)].z = z;
                
                velocity[lin(x,y,z)].x = 0;
                velocity[lin(x,y,z)].y = 0;
                velocity[lin(x,y,z)].z = 0;
                
                mass[lin(x,y,z)] = FLUID_MASS;
            }
        }
    }
}
