#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include <SDL.h>

#define MAX_SOURCE_SIZE (0x100000)


#ifdef CACHE
int** cached_points;
int** cached_x;
int** cached_y;
#endif

int *iteration_pixels;

typedef struct point_args point_args;
struct point_args
{
    int res_x;
    int res_y;
    int image_x;
    int image_y;
    float zoom;
    int max_iteration;
    int thread_number;
};

typedef struct piece_args piece_args;
struct piece_args
{
    int res_x;
    int res_y;
    float zoom;
    int max_iteration;
    int total_threads;
    int thread_number;
    int julia_mode;
};


int get_x (int linear_point, int width) 
{
    return linear_point % width;
}

int get_y (int linear_point, int height) 
{
    return floor(linear_point / height);
}

float map_x_mandelbrot(int x, int width, float zoom)
{
    return (((float)x / (float)width) * (3.5 * zoom)) - (2.5 - (1.0 - zoom));
    return (((float)x / (float)width) * (3.5 * zoom)) - (1.75 - (1.0 - zoom));
}

float map_x_julia(int x, int width, float zoom)
{
    return (((float)x / (float)width) * (3.5 * zoom)) - (1.75 - (1.0 - zoom));
}

float map_y(int y, int height, float zoom)
{
    return (((float)y / (float)height) * (2.0 * zoom)) - (1.00001 - (1.0 - zoom));
}

#ifdef CACHE
int cached_iteration(float pos_x, float pos_y)
{
    float centered_x = pos_x + 2.5;
    float centered_y = pos_y + 1.0;
    float temp_x = floor(centered_x * 1000.0);
    float temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    return cached_points[trs_pos_x][trs_pos_y];
}

float get_cached_x(float pos_x, float pos_y)
{
    float centered_x = pos_x + 2.5;
    float centered_y = pos_y + 1.0;
    float temp_x = floor(centered_x * 1000.0);
    float temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    return cached_x[trs_pos_x][trs_pos_y];
}

float get_cached_y(float pos_x, float pos_y)
{
    float centered_x = pos_x + 2.5;
    float centered_y = pos_y + 1.0;
    float temp_x = floor(centered_x * 1000.0);
    float temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    return cached_y[trs_pos_x][trs_pos_y];
}

void store_iteration(float pos_x, float pos_y, int iteration, float x, float y)
{
    float centered_x = pos_x + 2.5;
    float centered_y = pos_y + 1.0;
    float temp_x = floor(centered_x * 1000.0);
    float temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    cached_points[trs_pos_x][trs_pos_y] = iteration;
    cached_x[trs_pos_x][trs_pos_y] = x;
    cached_y[trs_pos_x][trs_pos_y] = y;
}
#endif

int mandelbrot_point(int res_x, int res_y, int image_x, int image_y, float zoom, int max_iteration)
{
    // Get the index of the current element
    float pos_x = map_x_mandelbrot(image_x, res_x, zoom);
    float pos_y = map_y(image_y, res_y, zoom);
    float x = 0.0;
    float y = 0.0;
    float q, x_term, pos_y2;
    float xtemp, xx, yy, xplusy;
#ifdef CACHE
    int storeable = 1;
#endif
    int iteration = 0;

    // Period-2 bulb check 
    x_term = pos_x + 1.0;
    pos_y2 = pos_y * pos_y;
    if ((x_term * x_term + pos_y2) < 0.0625) return 0;

    // Cardioid check
    x_term = pos_x - 0.25;
    q = x_term * x_term + pos_y2;
    q = q * (q + x_term);
    if (q < (0.25 * pos_y2)) return 0; 

#ifdef CACHE
    // Look up our cache
    iteration = cached_iteration(pos_x, pos_y);

    if (iteration > 0)
    {
        x = get_cached_x(pos_x, pos_y);
        y = get_cached_y(pos_x, pos_y);
        yy = y * y;
    }
    if (iteration < 0) storeable = 0;
#endif

    while (iteration < max_iteration)
    {
        xx = x * x;
        yy = y * y;
        xplusy = x + y;
        if ((xx) + (yy) > (4.0)) break;
        y = xplusy * xplusy - xx - yy;
        y = y + pos_y;
        xtemp = xx - yy + pos_x;

        x = xtemp;
        iteration++;
    }

    if (iteration >= max_iteration)
    {
        return 0;
    }
    else
    {
#ifdef CACHE
        if (storeable == 1)
        {
            store_iteration(pos_x, pos_y, iteration, x, y);
        }
#endif
        return iteration;
    }
}

int julia_point(int res_x, int res_y, int image_x, int image_y, float zoom, int max_iteration)
{
    // Get the index of the current element
    float pos_x = map_x_julia(image_x, res_x, 1.0);
    float pos_y = map_y(image_y, res_y, 1.0);
    float x = pos_x;
    float y = pos_y;
    float xtemp, xx, yy;
#ifdef CACHE
    int storeable = 1;
#endif
    int iteration = 0;

#ifdef CACHE
    // Look up our cache
    iteration = cached_iteration(pos_x, pos_y);

    if (iteration > 0)
    {
        x = get_cached_x(pos_x, pos_y);
        y = get_cached_y(pos_x, pos_y);
        yy = y * y;
    }
    if (iteration < 0) storeable = 0;
#endif

    while (iteration < max_iteration)
    {
        xx = x * x;
        yy = y * y;
        if ((xx) + (yy) > (4.0)) break;
        y = pow((x + y), 2) - xx - yy;
        y = y + 0.288;
        xtemp = xx - yy + 0.353 + zoom;

        x = xtemp;
        iteration++;
    }

    if (iteration >= max_iteration)
    {
        return 0;
    }
    else
    {
#ifdef CACHE
        if (storeable == 1)
        {
            store_iteration(pos_x, pos_y, iteration, x, y);
        }
#endif
        return iteration;
    }
}


// Splits the image in pieces and calls the corresponding algorithm
void *thread_launcher(void *arguments)
{
    piece_args *args;
    args = (piece_args *) arguments;

    int x,y, small_res_x, small_res_y, init_x, init_y, limit_x, limit_y;
    int split, piece_x, piece_y;

    if(args->total_threads > 2)
    {
        split = sqrt(args->total_threads);
    }
    else if (args->total_threads == 2)
    {
        split = 2;
    }
    else
    {
        split = 1;
    }

    if (args->thread_number > 0)
    {
        piece_x = args->thread_number % split;
        piece_y = floor((float)args->thread_number / (float)split);
    }
    else
    {
        piece_x = 0;
        piece_y = 0;
    }

    small_res_x = floor((float)args->res_x / (float)split);
    small_res_y = floor((float)args->res_y / (float)split);
    init_x = small_res_x * piece_x;
    init_y = small_res_y * piece_y;
    limit_x = init_x + small_res_x;
    limit_y = init_y + small_res_y;

    for (y = init_y; y < limit_y; y++)
    {
        for (x = init_x; x < limit_x; x++)
        {
            if(args->julia_mode == 0)
                iteration_pixels[x + (y * args->res_x)] = mandelbrot_point(args->res_x, args->res_y, x, y, args->zoom, args->max_iteration);
            else
                iteration_pixels[x + (y * args->res_x)] = julia_point(args->res_x, args->res_y, x, y, args->zoom, args->max_iteration);
        }
    }
}

int get_cpus()
{
    int number_of_cores = 0;
    number_of_cores = sysconf(_SC_NPROCESSORS_ONLN);
    return number_of_cores;
}

int main(int argn, char **argv) 
{
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());

    printf("SDL Initialized\n");

    // Create screen surface
    SDL_Surface *screen;
    int res_x = 800;
    int res_y = 600;
    int julia_mode = 0;

    if(argn == 1)
    {
        julia_mode = 0;
    } 
    else if ((argn == 2) && (strcmp(argv[1], "-julia") == 0))
    {
        julia_mode = 1;
    }

    int number_cores = get_cpus();
    int number_threads = number_cores * number_cores;

    printf("Number of CPUs/cores autodetected: %d\n", number_cores);

#ifdef CACHE
    // Init our cached points
    cached_points = malloc(res_y * 1000 * sizeof(int *));
    cached_x = malloc(res_y * 1000 * sizeof(float *));
    cached_y = malloc(res_y * 1000 * sizeof(float *));
    if (cached_points == NULL)
    {
        fprintf(stderr, "Bad luck, out of memory\n");
        return 2;
    }

    int count;
    for (count = 0; count < res_y * 1000; count++)
    {
        cached_points[count] = malloc(res_x * 1000 * sizeof(int));
        if(cached_points[count] == NULL)
        {
            fprintf(stderr, "Bad luck, out of memory\n");
            return 2;
        }
        cached_x[count] = malloc(res_x * 1000 * sizeof(float));
        cached_y[count] = malloc(res_x * 1000 * sizeof(float));
        /*for (count2 = 0; count2 < res_x * 100; count2++)
        {
            cached_points[count][count2] = -1;
        }*/
    }

    printf("Cache ready\n");
#endif

    // screen = SDL_SetVideoMode(res_x, res_y, 0, SDL_HWSURFACE|SDL_DOUBLEBUF);
    screen = SDL_SetVideoMode(res_x, res_y, 0, SDL_DOUBLEBUF);
    if(!screen)
	    fprintf(stderr,"Could not set video mode: %s\n",SDL_GetError());

    // Prepare the resolution and sizes and colors, threads...
    iteration_pixels = malloc(res_x * res_y * sizeof(int));
    pthread_t threads[number_threads];
    piece_args arguments[number_threads];

    printf("Rendering...\n");

    float zoom = 1.0;
    float stop_point;

    if (julia_mode == 0)
        stop_point = 0.00001;
    else
        stop_point = -2.5;

    while(zoom > stop_point)
    {
        int iteration, max_iteration, x, y, res;
        if((zoom < -0.02) && (zoom > -1.0))
        {
            max_iteration = 100;
        }
        else
        {
            max_iteration = 170;
        }

        int thread_count;

        for(thread_count = 0; thread_count < number_threads; thread_count++)
        {
            arguments[thread_count].res_x = res_x;
            arguments[thread_count].res_y = res_y;
            arguments[thread_count].zoom = zoom;
            arguments[thread_count].max_iteration = max_iteration;
            arguments[thread_count].total_threads = number_threads;
            arguments[thread_count].thread_number = thread_count;
            arguments[thread_count].julia_mode = julia_mode;
            pthread_create( &threads[thread_count], NULL, thread_launcher, (void*) &arguments[thread_count]);
        }

        for(thread_count = 0; thread_count < number_threads; thread_count++)
        {
            res = pthread_join(threads[thread_count], NULL);
            if (res != 0)
            {
                printf("Error in %d thread\n", thread_count);
            }
        }

        int rank;
        Uint32 *pixel;
        rank = screen->pitch/sizeof(Uint32);
        pixel = (Uint32*)screen->pixels;

        for(y = 0; y < res_y ; y++)
        {
            for(x = 0; x < res_x; x++)
            {
                iteration = iteration_pixels[x + y * res_x];
                if ((iteration < 128) && (iteration > 0)) {
                    pixel[x + y * rank] = SDL_MapRGBA(screen->format,
                                                       0,
                                                       20 + iteration,
                                                       0,
                                                       255);
                }
                else if ((iteration >= 128) && (iteration < max_iteration))
                {
                    pixel[x + y * rank] = SDL_MapRGBA(screen->format,
                                                       iteration,
                                                       148,
                                                       iteration,
                                                       255);
                }
                else
                {
                    pixel[x + y * rank] = SDL_MapRGBA(screen->format,
                                                       0,
                                                       0,
                                                       0,
                                                       255);
                } 
            }
        }

        if(julia_mode == 0)
            zoom = zoom * 0.99;
        else
            zoom -= 0.01; 

        SDL_Flip(screen);
    }

    // printf("Max Iteration value: %d\n", max_iter);

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

