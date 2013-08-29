float map_x(int x, int width)
{
    return (((float)x / (float)width) * 3.5) - 2.5;
}

float map_y(int y, int height)
{
    return (((float)y / (float)height) * 2.0) - 1.0;
}

__kernel void mandelbrot_point(__global const int res_x, __global const int res_y, __global const int line, __global int *graph_line) 
{
    // Get the index of the current element
    int image_x = get_global_id(0);
    int image_y = line;
    float pos_x = map_x(image_x, res_x);
    float pos_y = map_y(image_y, res_y);
    float x = 0.0;
    float y = 0.0;

    int iteration = 0;
    int max_iteration = 255;
    float xtemp;

    while (iteration < max_iteration)
    {
       xtemp = x * x - y * y + pos_x;
       y = 2.0 * x * y + pos_y;

       x = xtemp;
       iteration++;

       if ((x * x) + (y * y) >= (4.0)) break;
    }

    if (iteration >= max_iteration)
    {
       graph_line[image_x] = 0;
    }
    else
    {
       graph_line[image_x] = iteration;
    }
     
}
