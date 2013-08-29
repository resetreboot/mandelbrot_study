void main(void)
{
	float zoom = float(iMouse.x) / float(iResolution.x);
    float pos_x = ((float(gl_FragCoord.x) / float(iResolution.x)) * 3.5 * zoom) - (2.5 - (1.0 - zoom));
	float pos_y = ((float(gl_FragCoord.y) / float(iResolution.y)) * 2.0 * zoom) - (1.0 - (1.0 - zoom));
    float x = 0.0;
    float y = 0.0;

    int iteration = 0;
	float normal_iter = 0.0;
    int max_iteration = 255;
    float xtemp;

    while (iteration < max_iteration)
    {
       xtemp = x * x - y * y + pos_x;
       y = 2.0 * x * y + pos_y;
	   if ((x * x) + (y * y) >= (4.0)) break;

       x = xtemp;
       iteration++;
    }

    if (iteration >= max_iteration)
    {
		gl_FragColor = vec4(0.0,0.0,0.0,1.0);
    }
    else if (iteration < 128)
    {
		normal_iter = float(iteration) / 255.0;
        gl_FragColor = vec4(0,0.1 + normal_iter,normal_iter,1.0);
    }
	else
	{
		normal_iter = float(iteration) / 255.0;
		gl_FragColor = vec4(1.0 - normal_iter, 1.0, 1.0 - normal_iter, 1.0);
	}
}
