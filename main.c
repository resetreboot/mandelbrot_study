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

    screen = SDL_SetVideoMode(res_x, res_y, 0, SDL_HWSURFACE|SDL_DOUBLEBUF);
    if(!screen)
	    fprintf(stderr,"Could not set video mode: %s\n",SDL_GetError());

    // Prepare the resolution and sizes and colors...
    int i;
    int temp;
    const int ITERATIONS = 256;
    int *red_scale = (int*)malloc(sizeof(int)*ITERATIONS);
    int *blue_scale = (int*)malloc(sizeof(int)*ITERATIONS);
    for(i = 0; i < ITERATIONS; i++) {
        red_scale[i] = i;
        blue_scale[i] = 255 - i;   
    }

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
    cl_mem kernel_res_x = clCreateBuffer(context, CL_MEM_READ_ONLY,
            sizeof(cl_int), NULL, &ret);
    cl_mem kernel_res_y = clCreateBuffer(context, CL_MEM_READ_ONLY,
            sizeof(cl_int), NULL, &ret);
    cl_mem kernel_current_line = clCreateBuffer(context, CL_MEM_READ_ONLY,
            sizeof(cl_int), NULL, &ret);
    cl_mem graph_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
            res_x * sizeof(cl_int), NULL, &ret);

    // Copy resolution x and y for the kernel
    ret = clEnqueueWriteBuffer(command_queue, kernel_res_x, CL_TRUE, 0,
            sizeof(cl_int), &res_x, 0, NULL, NULL);

    ret = clEnqueueWriteBuffer(command_queue, kernel_res_y, CL_TRUE, 0,
            sizeof(cl_int), &res_y, 0, NULL, NULL);

    ret = clEnqueueWriteBuffer(command_queue, kernel_current_line, CL_TRUE, 0,
            sizeof(cl_int), &current_line, 0, NULL, NULL);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, 
            (const char **)&source_str, (const size_t *)&source_size, &ret);

    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "mandelbrot_point", &ret);

    // Our screen in a linear array
    int *graph_dots = (int*)malloc(total_res * sizeof(int)); 
    cl_int *graph_line = (cl_int*)malloc(res_x * sizeof(cl_int));

    for (current_line = 0; current_line < 600; current_line++)
    {
        // Set the arguments of the kernel
        ret = clEnqueueWriteBuffer(command_queue, kernel_current_line, CL_TRUE, 0,
                sizeof(cl_int), &current_line, 0, NULL, NULL);

        ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &kernel_res_x);
        ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &kernel_res_y);
        ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), &kernel_current_line);
        ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), graph_mem_obj);
        
        // Execute the OpenCL kernel on the list
        size_t global_item_size = res_x; // Process the entire screen
        size_t local_item_size = 64; // Process in groups of 64
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
                &global_item_size, &local_item_size, 0, NULL, NULL);

        // Read the memory buffer graph_mem_obj on the device to the local variable graph_dots
        ret = clEnqueueReadBuffer(command_queue, graph_mem_obj, CL_TRUE, 0, 
                res_x * sizeof(cl_int), graph_line, 0, NULL, NULL);

        for (i = 0; i < 800; i++)
        {
            graph_dots[(current_line * 800) + i] = graph_line[i];
        }
    }

    // Display the result to the screen
    /* for(i = 0; i < 3078; i++)
        printf("Linear: %d -> %d\n", i, graph_dots[i]); */

    printf("Rendering...\n");

    int iteration;
	Uint32 *pixel;
	// Lock surface
	SDL_LockSurface(screen);
	// rank = screen->pitch/sizeof(Uint32);
	pixel = (Uint32*)screen->pixels;
	/* Draw all dots */
	for(i = 0;i < total_res;i++)
	{
		// Get the iterations for the point
        // printf("Point %d\n", i);
        iteration = graph_dots[i];
        if ((iteration < 1000) && (iteration >= 0)) {
            pixel[i] = SDL_MapRGBA(screen->format,
                                   red_scale[iteration],
                                   0,
                                   blue_scale[iteration],
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
	SDL_UnlockSurface(screen);

    // Draw to the scree
    SDL_Flip(screen);

    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    // ret = clReleaseMemObject(a_mem_obj);
    // ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseMemObject(graph_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    // free(A);
    // free(B);
    free(graph_dots);

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

