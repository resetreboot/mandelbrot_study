#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <SDL.h>

#define MAX_SOURCE_SIZE (0x100000)

int main(int argn, char **argv) {
    
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());

    printf("SDL Initialized\n");

    // Create screen surface
    SDL_Surface *screen;
    int res_x = 800;
    int res_y = 600;
    int current_line = 0;
    int total_res = res_x * res_y;

    screen = SDL_SetVideoMode(res_x, res_y, 0, SDL_DOUBLEBUF);
    if(!screen)
	    fprintf(stderr,"Could not set video mode: %s\n",SDL_GetError());

    // Prepare the resolution and sizes and colors...
    int i;
    const int ITERATIONS = 256;

    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("mandelbrot_kernel.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose( fp );

    // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;   
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_GPU, 1, 
            &device_id, &ret_num_devices);

    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);

    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

    // Create memory buffers on the device for returning iterations 
    // Input parameters
    cl_mem kernel_res_x = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(int), &res_x, &ret);
    cl_mem kernel_res_y = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(int), &res_y, &ret);
    cl_mem kernel_current_line = clCreateBuffer(context, CL_MEM_READ_ONLY,
            sizeof(int), NULL, &ret);
    cl_mem kernel_zoom_level = clCreateBuffer(context, CL_MEM_READ_ONLY,
            sizeof(float), NULL, &ret); 
    // Output buffer
    cl_mem graph_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
            res_x * sizeof(int), NULL, &ret);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, 
            (const char **)&source_str, (const size_t *)&source_size, &ret);

    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

    // Check if it is correct
    printf("clBuildProgram\n");
    cl_build_status build_status;
    ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL);

    char *build_log;
    size_t ret_val_size;
    ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);

    build_log = (char *) malloc((ret_val_size + 1) * sizeof(char));
    ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
                                build_log[ret_val_size] = '\0';
    printf("BUILD LOG: \n %s", build_log);
    printf("program built\n");

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "mandelbrot_point", &ret);

    // Common kernel params
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &kernel_res_x);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &kernel_res_y);
    ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *) &graph_mem_obj);

    // Our screen in a linear array
    int graph_dots[total_res]; // = (int*)malloc(total_res * sizeof(int)); 
    int graph_line[res_x];  // (int*)malloc(res_x * sizeof(int));
    float zoom;             // Our current zoom level

    for(zoom = 1.0; zoom > 0.00001; zoom = zoom * 0.98)
    {
        for (current_line = 0; current_line < res_y; current_line++)
        {
            // Set the arguments of the kernel
            ret = clEnqueueWriteBuffer(command_queue, kernel_current_line, CL_TRUE, 0,
                    sizeof(int), &current_line, 0, NULL, NULL);

            ret = clEnqueueWriteBuffer(command_queue, kernel_zoom_level, CL_TRUE, 0,
                    sizeof(float), &zoom, 0, NULL, NULL);

            clFinish(command_queue);

            ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &kernel_current_line);
            ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *) &kernel_zoom_level);
            
            // Execute the OpenCL kernel on the list
            size_t global_item_size = res_x; // Process the entire line
            size_t local_item_size = 32; // Process in groups of 64
            ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
                    &global_item_size, &local_item_size, 0, NULL, NULL);

            if (ret != CL_SUCCESS)
            {
                printf("Error while executing kernel\n");
                printf("Error code %d\n", ret);
            }

            // Wait for the computation to finish
            clFinish(command_queue);

            // Read the memory buffer graph_mem_obj on the device to the local variable graph_dots
            ret = clEnqueueReadBuffer(command_queue, graph_mem_obj, CL_TRUE, 0, 
                    res_x * sizeof(int), graph_line, 0, NULL, NULL);

            if (ret != CL_SUCCESS)
                printf("Error while reading results buffer\n");

            clFinish(command_queue);

            int line_count;
            for (line_count = 0; line_count < res_x; line_count++)
            {
                int temp_val = graph_line[line_count];
                graph_dots[(current_line * res_x) + line_count] = temp_val;
            }
        }

        int iteration;
        Uint32 *pixel;
        // Lock surface
        // SDL_LockSurface(screen);
        // rank = screen->pitch/sizeof(Uint32);
        pixel = (Uint32*)screen->pixels;
        /* Draw all dots */
        for(i = 0;i < total_res;i++)
        {
            // Get the iterations for the point
            // printf("Point %d\n", i);
            iteration = graph_dots[i];
            if ((iteration < 128) && (iteration > 0)) {
                pixel[i] = SDL_MapRGBA(screen->format,
                                       0,
                                       20 + iteration,
                                       0,
                                       255);
            }
            else if ((iteration >= 128) && (iteration < ITERATIONS))
            {
                pixel[i] = SDL_MapRGBA(screen->format,
                                       iteration,
                                       148,
                                       iteration,
                                       255);
            }
            else
            {
                pixel[i] = SDL_MapRGBA(screen->format,
                                                   0,
                                                   0,
                                                   0,
                                                   255);
            } 
        }
        // Unlock surface
        // SDL_UnlockSurface(screen);

        // Draw to the scree
        SDL_Flip(screen);
    }

    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(kernel_res_x);
    ret = clReleaseMemObject(kernel_res_y);
    ret = clReleaseMemObject(kernel_current_line);
    ret = clReleaseMemObject(graph_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    // free(A);
    // free(B);
    // free(graph_dots);
    // free(graph_line);

    SDL_Event ev;
    int active;

    active = 1;
    while(active)
    {
        /* Handle events */
        while(SDL_PollEvent(&ev))
        {
            if(ev.type == SDL_QUIT)
                active = 0; /* End */
        }
    }

    SDL_Quit();

    return 0;
}

