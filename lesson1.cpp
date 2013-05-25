/*Code gratefully stollen from 
http://developer.amd.com/tools/heterogeneous-computing/amd-accelerated-parallel-processing-app-sdk/introductory-tutorial-to-opencl/
*/

#include<utility>
#define __NO_STD_VECTOR // use cl::vector instead of STL version
#include <CL/cl.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>

const std::string hw("111111111111");//this shouldn't actually matter,just for size
//const std::string hw("Hello World\n");//this is  the original

inline void
checkErr(cl_int err, const char * name)
{
    if (err != CL_SUCCESS) {
        std::cerr << "ERROR: " << name
        << " (" << err << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
}

int
main(void)
{
    cl_int err;
    
    //A platform is a specific implementation of OpenCL by a vendor
    cl::vector< cl::Platform > platformList;    
    //let's get a list of them
    cl::Platform::get(&platformList);
    checkErr(platformList.size() != 0 ? CL_SUCCESS : -1, "cl::Platform::get");
    //see how many we get
    std::cerr << "Platform number is: " << platformList.size() <<
        std::endl;
    std::string platformVendor;
    //get some information about the first platform's vendor (the one we'll use)
    platformList[0].getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &platformVendor);
    std::cerr << "Platform vendor is: " << platformVendor << std::endl;
    
    /*A context is where everything happens (creating devices and memory,
     *compiling and running programs). They can have associated devices (CPU
     *or GPU), and OpenCL guarantees some form of relaxed memory consistency
     *within.
     */
     
    //make the context properties    
    cl_context_properties cprops[3] =
        {CL_CONTEXT_PLATFORM, (cl_context_properties)(platformList[0])(), 0};
    //make the context
    cl::Context context(
        CL_DEVICE_TYPE_CPU,
        cprops,
        NULL,
        NULL,//fjdksljfdksljfdsklj
        &err);
    checkErr(err, "Context::Context()");

    //allocate an OpenCL Buffer to hold the result of the kernel to be run
    //on the device. 
    int * outH = new int[13];
    int inp[] = {1, 2, 3,4,5,6,7,8,9,10,11,12,13};
    //we'll allocate on the host and ask OpenCL to use this directly, with
    //CL_MEM_USE_HOST_PTR
    cl::Buffer outCL(
        context,
        CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
        13,
        outH,
        &err);

    cl::Buffer inCL(
        context,
        CL_MEM_COPY_HOST_PTR,
        13,
        inp,
        &err);
    checkErr(err, "Buffer::Buffer()");

    //most work is done on a context-wide basis, but some is done on a device-wide
    //basis, such as compiling or running a kernel
    cl::vector<cl::Device> devices;
    //we query the context to get a handle for the device
    devices = context.getInfo<CL_CONTEXT_DEVICES>();
    checkErr(devices.size() > 0 ? CL_SUCCESS : -1, "devices.size() > 0");
    
        
    //all device computations are done using a command queue, a virtual
    //interface for each device in question (one cqueue per device)
    cl::CommandQueue queue(context, devices[0], 0, &err);
    checkErr(err, "CommandQueue::CommandQueue()");
    cl::Event event;
    err = queue.enqueueWriteBuffer(
        inCL,
        CL_TRUE,
        0,
        13,
        inp,
        NULL,
        &event);
    checkErr(err, "ComamndQueue::enqueueWriteBuffer()");
    event.wait();

    //now we need to load and build the compute program (thing to run on device)
    std::ifstream file("lesson1_kernels.cl");
    checkErr(file.is_open() ? CL_SUCCESS:-1, "lesson1_kernel.cl");
    //convert to string
    std::string prog(
        std::istreambuf_iterator<char>(file),
        (std::istreambuf_iterator<char>()));
    //make a Source object from it
    cl::Program::Sources source(1,
        std::make_pair(prog.c_str(), prog.length()+1));
    //a program object is created and associated with a context
    cl::Program program(context, source);
    //we build the program with for a particular set of devices
    err = program.build(devices,"");
    checkErr(err, "Program::build()");
    

    //a given program can have many entry points called kernels
    //we have to build these kernel objects
    //The string we use ("hello") should be the name of a __kernel function
    //somewhere in the compute program
    cl::Kernel kernel(program, "hello", &err);
    checkErr(err, "Kernel::Kernel()");
    //we set its arguments using kernel.setArg(), takes index and value
    //for a particular argument
    //note that outCL was defined as a buffer earlier
    err = kernel.setArg(0, outCL);
    err = kernel.setArg(1, inCL);
    checkErr(err, "Kernel::setArg()");
    
    /*
     * TIME TO ACTUALLY DO SHIZ
     */

    
    //once you have a commandqueue, you can queue kernels using a queue.enqueueNDRangeKernel
    //You can choose the number of work-items per work-group, and dimensions
    err = queue.enqueueNDRangeKernel(
        kernel,
        cl::NullRange,//dimensions
        cl::NDRange(13),//total number of work-items (in dimensions)
        cl::NDRange(1, 1),//number of work-items per work group (in dimensions)
        NULL,
        &event);//so we can query the status of the command (to see if it is completed)
    checkErr(err, "ComamndQueue::enqueueNDRangeKernel()");
    
    //now we wait for the compute program to finish
    event.wait();
    //and now we read back the result from outCL to outH
    err = queue.enqueueReadBuffer(
        outCL,
        CL_TRUE,
        0,
        13,
        outH);
    checkErr(err, "ComamndQueue::enqueueReadBuffer()");
    //and print it out
    for (int i = 0; i < 13; i++) {
        std::cout << outH[i] << " ";
    }
    //std::cout << *outH;
    return EXIT_SUCCESS;
}
