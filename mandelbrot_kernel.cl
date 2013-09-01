float map_x(int x, int width, float zoom)
{
    return (((float)x / (float)width) * (3.5 * zoom)) - (2.5 - (1.0 - zoom));
}

float map_y(int y, int height, float zoom)
{
    return (((float)y / (float)height) * (2.0 * zoom)) - (1.00001 - (1.0 - zoom));
}

__kernel void fractal_point(__global const int *res_x, 
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
    float q, x_term;

    // Period-2 bulb check 
    if (((pos_x + 1.0) * (pos_x + 1.0) + pos_y * pos_y) < 0.0625)
    {
        graph_line[image_x] = 0;
        return;
    }

    // Cardioid check
    x_term = pos_x - 0.25;
    q = x_term * x_term + pos_y * pos_y;
    q = q * (q + x_term);
    if (q < (0.25 * pos_y * pos_y))
    {
        graph_line[image_x] = 0;
        return;
    }

    int iteration = 0;
    int max_iteration = 256;
    float xtemp, xx, yy, xplusy;

    while (iteration < max_iteration)
    {
       xx = x * x;
       yy = y * y;
       xplusy = x + y;
       if ((xx) + (yy) > (4.0)) break;

       xtemp = xx - yy + pos_x;
       y = xplusy * xplusy - xx - yy;
       y = y + pos_y;

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
