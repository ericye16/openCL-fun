

#include <stdio.h>
#include <stdlib.h>
#include<stdbool.h>

// OpenCL includes
#include <CL/cl.h>

#define FLUID_MASS 13.f //highly arbitrary

const int xSize = 8;
const int ySize = 8;
const int zSize = 8;
const cl_float mu = 0.2f;
const cl_float gasConstant = .8f;

const cl_float3 gravity = {0.f, -1.f, 0.f};

int lin(int x, int y, int z); 

void initialize(cl_float3 * position, cl_float3 * velocity, cl_float * mass);

void checkErr(int err) {
    if (err != CL_SUCCESS) {
        fprintf(stderr, "SOMETHING TERRIBLE HAPPENED: %i\n", err);
    }
}

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
    cl_float * pressure = NULL;
    //cl_float * result_dot = NULL;
    
    // Compute the size of the data 
    size_t datasize_vel = sizeof(cl_float3) * elements;
    size_t datasize_pos = sizeof(cl_float3) * elements;
    size_t datasize_pressure = sizeof(cl_float) * elements;

    // Allocate space for input/output data
    
    //initialize our data
    position = (cl_float3*)malloc(datasize_pos);
    velocity = (cl_float3*)malloc(datasize_vel);
    mass = (cl_float*)malloc(datasize_pressure);
    velocity_final = (cl_float3*)malloc(datasize_vel);
    position_final = (cl_float3*)malloc(datasize_pos);
    pressure = (cl_float*)malloc(datasize_pressure);
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
    checkErr(status);

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
    checkErr(status);
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
    cl_mem bufferVelFinal;
    cl_mem bufferPosFinal;
    cl_mem bufferMass;
    cl_mem bufferDensity;
    cl_mem bufferPressure;
    cl_mem bufferMu;
    cl_mem bufferGravity;
    cl_mem bufferGasConstant;
    
    bufferVel = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        datasize_vel,
        NULL,
        &status);
    checkErr(status);
    bufferPos = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        datasize_pos,
        NULL,
        &status);
    checkErr(status);
    
    bufferVelFinal = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        datasize_vel,
        NULL,
        &status);
    
    checkErr(status);
    bufferPosFinal = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        datasize_pos,
        NULL,
        &status);
        
    checkErr(status);
    bufferMass = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        datasize_pressure,
        NULL,
        &status);
        
    checkErr(status);
    bufferDensity = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        datasize_pressure,
        NULL,
        &status);
    
    checkErr(status);
    bufferPressure = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        datasize_pressure,
        NULL,
        &status);
    checkErr(status);
    
    bufferMu = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(cl_float),
        NULL,
        &status);
    checkErr(status);
    
    bufferGravity = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(cl_float3),
        NULL,
        &status);
    checkErr(status);
    
    bufferGasConstant = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(cl_float),
        NULL,
        &status);
    checkErr(status);
    
    
    fprintf(stderr, "Buffers created\n");
    
        
    //-----------------------------------------------------
    // STEP 6: Write host data to device buffers
    //----------------------------------------------------- 
    
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
    
    status = clEnqueueWriteBuffer(
        cmdQueue,
        bufferMass,
        CL_FALSE,
        0,
        datasize_pressure,
        mass,
        0,
        NULL,
        NULL);
    checkErr(status);
    
    status = clEnqueueWriteBuffer(
        cmdQueue,
        bufferMu,
        CL_FALSE,
        0,
        sizeof(cl_float),
        &mu,
        0,
        NULL,
        NULL);
    checkErr(status);
    
    status = clEnqueueWriteBuffer(
        cmdQueue,
        bufferGasConstant,
        CL_FALSE,
        0,
        sizeof(cl_float),
        &gasConstant,
        0,
        NULL,
        NULL);
    checkErr(status);
    
    status = clEnqueueWriteBuffer(
        cmdQueue,
        bufferGravity,
        CL_FALSE,
        0,
        sizeof(cl_float3),
        &gravity,
        0,
        NULL,
        NULL);
    checkErr(status);
    
    
        
    
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
    checkErr(status);
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
    checkErr(status);
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
    checkErr(status);
    
    status = clSetKernelArg(
        kernel, 
        1, 
        sizeof(cl_mem), 
        &bufferPos);
    checkErr(status);
    
    status = clSetKernelArg(
        kernel, 
        2, 
        sizeof(cl_mem), 
        &bufferMass);
    checkErr(status);
    
    status = clSetKernelArg(
        kernel,
        3,
        sizeof(cl_mem),
        &bufferDensity);
    checkErr(status);

    status = clSetKernelArg(
        kernel,
        4,
        sizeof(cl_mem),
        &bufferPressure);
    checkErr(status);
    
    status = clSetKernelArg(
        kernel,
        5,
        sizeof(cl_mem),
        &bufferGasConstant);
    checkErr(status);
    
    status = clSetKernelArg(
        kernel,
        6,
        sizeof(cl_mem),
        &bufferMu);
    checkErr(status);
    
    status = clSetKernelArg(
        kernel,
        7,
        sizeof(cl_mem),
        &bufferGravity);
    checkErr(status);
    
    status = clSetKernelArg(
        kernel,
        8,
        sizeof(cl_mem),
        &bufferVelFinal);
    checkErr(status);
    
    status = clSetKernelArg(
        kernel,
        9,
        sizeof(cl_mem),
        &bufferPosFinal);    
    checkErr(status);
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
    
    cl_event event;
    status = clEnqueueNDRangeKernel(
        cmdQueue, 
        kernel, 
        numDims, 
        NULL, 
        globalWorkSize, 
        localWorkSize, 
        0, 
        NULL, 
        &event);
    if (status != CL_SUCCESS) {
        fprintf(stderr, "Kernels not launched.");
    }
    fprintf(stderr, "Kernel launched and run.\n");
    
    clWaitForEvents(1, &event);
    fprintf(stderr, "Kernel event finished running\n");

    //-----------------------------------------------------
    // STEP 12: Read the output buffer back to the host
    //----------------------------------------------------- 
    
    // Use clEnqueueReadBuffer() to read the OpenCL output  
    // buffer (bufferC) 
    // to the host output array (C)
    
    status = clEnqueueReadBuffer(
        cmdQueue, 
        bufferPosFinal, 
        CL_TRUE, 
        0, 
        datasize_pos, 
        position_final, 
        0, 
        NULL, 
        NULL);
    checkErr(status);
    fprintf(stderr, "Position read out\n");
    
    status = clEnqueueReadBuffer(
        cmdQueue,
        bufferVelFinal,
        CL_TRUE,
        0,
        datasize_vel,
        velocity_final,
        0,
        NULL,
        NULL);
    checkErr(status);

    cl_float3 thisPos;
    cl_float3 thisVel;
    // Verify the output
    for (int x = 0; x < xSize; x++) {
        for (int y = 0; y < ySize; y++) {
            for (int z = 0; z < zSize; z++) {
                thisPos = position_final[lin(x, y, z)];
                thisVel = velocity_final[lin(x, y, z)];
                printf("(%f, %f, %f) at (%f, %f, %f) from (%i, %i, %i)\n",
                    thisPos.x, thisPos.y, thisPos.z,
                    thisVel.x, thisVel.y, thisVel.z,
                    x, y, z);
            }
        }
    }

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
    clReleaseMemObject(bufferVel);
    clReleaseMemObject(bufferPos);
    clReleaseMemObject(bufferVelFinal);
    clReleaseMemObject(bufferMass);
    clReleaseMemObject(bufferDensity);
    clReleaseMemObject(bufferPressure);
    clReleaseMemObject(bufferMu);
    clReleaseMemObject(bufferGravity);
    clReleaseMemObject(bufferGasConstant);
    clReleaseContext(context);
    fprintf(stderr, "Released kernel, program and queue\n");

    // Free host resources
    free(velocity);
    fprintf(stderr, "Released velocity\n");
    free(position);
    fprintf(stderr, "Released position\n");
    free(mass);
    fprintf(stderr, "Released mass\n");
    free(position_final);
    fprintf(stderr, "Released final position\n");
    free(velocity_final);
    fprintf(stderr, "Released final velocity\n");
    free(pressure);
    fprintf(stderr, "Released pressure\n");

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
