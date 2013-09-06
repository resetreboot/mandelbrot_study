#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <time.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <SDL.h>
#include <SDL_ttf.h>

#define MAX_SOURCE_SIZE (0x100000)

int main(int argn, char **argv) {
    
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());

    printf("SDL Initialized\n");

    // Create screen surface
    SDL_Surface *screen, *message;
    int res_x = 800;
    int res_y = 600;
    int current_line = 0;
    int julia_mode = 0;

    if (argn == 1)
    {
        julia_mode = 0;
    }
    else if ((argn == 2) && (strcmp(argv[1], "-julia") == 0))
    {
        julia_mode = 1;
        printf("Julia mode activated...\n");
    }

    SDL_Window *window = SDL_CreateWindow("CLFract",
                                           SDL_WINDOWPOS_UNDEFINED,
                                           SDL_WINDOWPOS_UNDEFINED,
                                           res_x, res_y, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    if ((!window) || (!renderer))
	    fprintf(stderr,"Could not set video mode: %s\n",SDL_GetError());

    // Blank the window
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Texture *texture_screen = SDL_CreateTexture(renderer,
                                                   SDL_PIXELFORMAT_ARGB8888,
                                                   SDL_TEXTUREACCESS_STREAMING,
                                                   res_x, res_y);

    screen = SDL_CreateRGBSurface(0, res_x, res_y , 32, 0, 0, 0, 0);

    //Initialize SDL_ttf
    if( TTF_Init() == -1 )
    { 
        printf("Error setting up TTF module.\n");
        return 1; 
    }

    // Load a font
    TTF_Font *font;
    font = TTF_OpenFont("font.ttf", 24);
    if (font == NULL)
    {
        printf("TTF_OpenFont() Failed: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    //The color of the font 
    SDL_Color textColor = { 255, 255, 255 };

    // Prepare the resolution and sizes and colors...
    const int ITERATIONS = 256;

    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;

    if (julia_mode == 0)
        fp = fopen("mandelbrot_kernel.cl", "r");
    else
        fp = fopen("julia_kernel.cl", "r");

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
    cl_kernel kernel = clCreateKernel(program, "fractal_point", &ret);

    // Common kernel params
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &kernel_res_x);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &kernel_res_y);
    ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *) &graph_mem_obj);

    int graph_line[res_x];  // (int*)malloc(res_x * sizeof(int));
    float zoom = 1.0;             // Our current zoom level
    float stop_point;

    if (julia_mode == 0)
        stop_point = 0.00001;
    else
        stop_point = -2.5;

    clock_t start = clock();

    while(zoom > stop_point) 
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
                // printf("Error while executing kernel\n");
                // printf("Error code %d\n", ret);
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
            Uint32 *pixel;
            // Lock surface
            // SDL_LockSurface(screen);
            // rank = screen->pitch/sizeof(Uint32);
            pixel = (Uint32*)screen->pixels;
            int iteration;

            for (line_count = 0; line_count < res_x; line_count++)
            {
                // int temp_val = graph_line[line_count];
                // graph_dots[(current_line * res_x) + line_count] = temp_val;
                // Get the iterations for the point
                // printf("Point %d\n", i);
                iteration = graph_line[line_count];
                if ((iteration < 128) && (iteration > 0)) {
                    pixel[(current_line * res_x) + line_count] = SDL_MapRGBA(screen->format,
                                           0,
                                           20 + iteration,
                                           0,
                                           255);
                }
                else if ((iteration >= 128) && (iteration < ITERATIONS))
                {
                    pixel[(current_line * res_x) + line_count] = SDL_MapRGBA(screen->format,
                                           iteration,
                                           148,
                                           iteration,
                                           255);
                }
                else
                {
                    pixel[(current_line * res_x) + line_count] = SDL_MapRGBA(screen->format,
                                                       0,
                                                       0,
                                                       0,
                                                       255);
                } 
            }
        }

        // Step, iterate our zoom levels if we're doing mandelbrot or julia set
        if (julia_mode == 0)
            zoom = zoom * 0.98;
        else
            zoom -= 0.01;

        // Draw message on a corner...
        char* msg = (char *)malloc(100 * sizeof(char));
        sprintf(msg, "Zoom level: %0.3f", zoom * 100.0);
        message = TTF_RenderText_Solid( font, msg, textColor );
        free(msg);
        if (message != NULL)
            SDL_BlitSurface(message, NULL, screen, NULL);

        free(message);

        SDL_UpdateTexture(texture_screen, NULL, (Uint32*)screen->pixels, 800 * sizeof (Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture_screen, NULL, NULL);
        SDL_RenderPresent(renderer);
        // Draw to the screen
        // SDL_Flip(screen);
    }

    printf("Time elapsed %0.5f seconds\n", ((double)clock() - start) / CLOCKS_PER_SEC);

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

