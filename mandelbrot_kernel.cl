float map_x(int x, int width, float zoom)
{
    return (((float)x / (float)width) * (3.5 * zoom)) - (2.5 - (1.0 - zoom));
}

float map_y(int y, int height, float zoom)
{
    return (((float)y / (float)height) * (2.0 * zoom)) - (1.00001 - (1.0 - zoom));
}

__kernel void mandelbrot_point(__global const int *res_x, 
                               __global const int *res_y, 
                               __global const int *line, 
                               __global const float *zoom, 
                               __global int *graph_line) 
{
    // Get the index of the current element
    int image_x = get_global_id(0);
    int image_y = *line;
    float pos_x = map_x(image_x, *res_x, *zoom);
    float pos_y = map_y(image_y, *res_y, *zoom);
    float x = 0.0;
    float y = 0.0;

    int iteration = 0;
    int max_iteration = 256;
    float xtemp, xx, yy;

    while (iteration < max_iteration)
    {
       xx = x * x;
       yy = y * y;
       if ((xx) + (yy) > (4.0)) break;

       xtemp = xx - yy + pos_x;
       y = 2.0 * x * y + pos_y;

       x = xtemp;
       iteration++;
    }

    if (iteration > max_iteration)
    {
       graph_line[image_x] = 0;
    }
    else
    {
       graph_line[image_x] = iteration;
    }
}
