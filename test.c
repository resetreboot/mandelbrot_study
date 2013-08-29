#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL.h>

#define MAX_SOURCE_SIZE (0x100000)


#ifdef CACHE
int** cached_points;
int** cached_x;
int** cached_y;
#endif

int get_x (int linear_point, int width) 
{
    return linear_point % width;
}

int get_y (int linear_point, int height) 
{
    return floor(linear_point / height);
}

double map_x(int x, int width, double zoom)
{
#ifndef JULIA
    return (((double)x / (double)width) * (3.5 * zoom)) - (2.5 - (1.0 - zoom));
#else
    return (((double)x / (double)width) * (3.5 * zoom)) - (1.7 - (1.0 - zoom));
#endif
}

double map_y(int y, int height, double zoom)
{
    return (((double)y / (double)height) * (2.0 * zoom)) - (1.0 - (1.0 - zoom));
}

#ifdef CACHE
int cached_iteration(double pos_x, double pos_y)
{
    double centered_x = pos_x + 2.5;
    double centered_y = pos_y + 1.0;
    double temp_x = floor(centered_x * 1000.0);
    double temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    return cached_points[trs_pos_x][trs_pos_y];
}

double get_cached_x(double pos_x, double pos_y)
{
    double centered_x = pos_x + 2.5;
    double centered_y = pos_y + 1.0;
    double temp_x = floor(centered_x * 1000.0);
    double temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    return cached_x[trs_pos_x][trs_pos_y];
}

double get_cached_y(double pos_x, double pos_y)
{
    double centered_x = pos_x + 2.5;
    double centered_y = pos_y + 1.0;
    double temp_x = floor(centered_x * 1000.0);
    double temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    return cached_y[trs_pos_x][trs_pos_y];
}

void store_iteration(double pos_x, double pos_y, int iteration, double x, double y)
{
    double centered_x = pos_x + 2.5;
    double centered_y = pos_y + 1.0;
    double temp_x = floor(centered_x * 1000.0);
    double temp_y = floor(centered_y * 1000.0);

    int trs_pos_x = (int)temp_x;
    int trs_pos_y = (int)temp_y;

    cached_points[trs_pos_x][trs_pos_y] = iteration;
    cached_x[trs_pos_x][trs_pos_y] = x;
    cached_y[trs_pos_x][trs_pos_y] = y;
}
#endif

int mandelbrot_point(int res_x, int res_y, int image_x, int image_y, double zoom, int max_iteration)
{
    return abs(floor(sin(((double)image_x + zoom) * 0.1) * 127) + floor(cos(((double)image_y + zoom) * 0.1) * 127));
}

int julia_point(int res_x, int res_y, int image_x, int image_y, double zoom, int max_iteration)
{
    // Get the index of the current element
    double pos_x = map_x(image_x, res_x, 1.0);
    double pos_y = map_y(image_y, res_y, 1.0);
    double x = pos_x;
    double y = pos_y;
    double q, x_term;
    double xtemp, xx, yy;
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

int main(int argn, char **argv) {
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());

    printf("SDL Initialized\n");

    // Create screen surface
    SDL_Surface *screen;
    int res_x = 800;
    int res_y = 600;
    int total_res = res_x * res_y;

#ifdef CACHE
    // Init our cached points
    cached_points = malloc(res_y * 1000 * sizeof(int *));
    cached_x = malloc(res_y * 1000 * sizeof(double *));
    cached_y = malloc(res_y * 1000 * sizeof(double *));
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
        cached_x[count] = malloc(res_x * 1000 * sizeof(double));
        cached_y[count] = malloc(res_x * 1000 * sizeof(double));
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

    // Prepare the resolution and sizes and colors...
    int i;
    int temp;
    const int ITERATIONS = 256;

    printf("Rendering...\n");

    double zoom;

#ifndef JULIA
    for (zoom = 1.0; zoom < 200.0 ; zoom += 1.0)
#else
    for (zoom = 1.0; zoom > -2.5 ; zoom -= 0.01)
#endif
    {
        i = 0;
        int iteration, max_iteration, x, y;
        if((zoom < -0.02) && (zoom > -1.0))
        {
            max_iteration = 100;
        }
        else
        {
            max_iteration = 255;
        }
        int col_value;
        Uint32 *pixel;
        int rank;
        // Lock surface
        // SDL_LockSurface(screen);
        rank = screen->pitch/sizeof(Uint32);
        pixel = (Uint32*)screen->pixels;
        /* Draw all dots */
        for(y = 0;y < res_y;y++)
        {
            for(x = 0;x < res_x;x++)
            {
                #ifndef JULIA
                iteration = mandelbrot_point(res_x, res_y, x, y, zoom, max_iteration);
                #else
                iteration = julia_point(res_x, res_y, x, y, zoom, max_iteration);
                #endif
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
                                           200,
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
                i++;
            }       
        }
        // Unlock surface
        // SDL_UnlockSurface(screen);

        // Draw to the screen
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

