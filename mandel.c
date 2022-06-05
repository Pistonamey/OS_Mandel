/*
	Name: Amey Shinde
    ID: 1001844387

    Comments for Grader - Code used is from single-threaded.c, multi-threaded.c and time.c 
	   					  on professor's gihub repo. 
                          i' and 'j' are not used as loop variable 
                          because acc to prof, i and j are used for complex numbers
*/


#include "bitmap.h"

#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//struct that holds the minimum width,
//maximum width, minimum height,
//maximum height of the image bands that need 
//to be threaded along with their 
//scale and the number of threads
struct Parameters{ 
	int x_min; 
	int x_max; 
	int n_threads;
	int y_min; 
	int y_max; 
	float scale; 	
}; 

//struct array that holds all the parameters
//for every band 
struct Parameters param_struct[1000];

//pointer to our struct array
struct Parameters* param_struct_ptr = param_struct;

//bitmap of the appropriate size for 
//storing the image
struct bitmap *bm; 

//function that takes all the image parameters
//and assigns the minimum width,
//maximum width, minimum height,
//maximum height for their particular image bands
//based on the number of threads
struct Parameters* create_array_struct( int n_threads, struct Parameters* param_struct_ptr, 
			int image_width, int image_height, float scale, int num_bands );

//function to get the number of bands for computing
//the image using threading
int get_num_bands( int image_height, int n_threads );
 
int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void *compute_image( void *paramsarg ); 

int max;
float x_center; 
float scale;
const char *outfile;
int image_height; 
float y_center; 
int n_threads; 
int image_width; 
	
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-n <threads>    Number of threads. (default=1)\n"); 
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");

	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] )
{	
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	outfile = "mandel.bmp";
	x_center = 0;
	y_center = 0;
	scale = 4;
	n_threads = 1; 
	image_width = 500;
	image_height = 500;
	max = 1000;
	

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:n:o:h"))!=-1) {
		switch(c) {
			case 'x':
				x_center = atof(optarg);
				break;
			case 'y':
				y_center = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'n': //override the number of threads as specified by the user
				n_threads = atoi(optarg); 
				break; 
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

	int num_lines = get_num_bands(image_height, n_threads); 

	
	pthread_t td[1000] = {0};  


	struct timeval end_time; 
	long int time; 
	struct timeval begin_time; 

	
	param_struct_ptr = create_array_struct(n_threads, param_struct_ptr, image_width, 
					   image_height, scale, num_lines);   	
	
	
	// Display the configuration of the image.
	printf("\nmandel: x=%lf y=%lf scale=%lf max=%d threads=%d, outfile=%s\n"
		,x_center,y_center,scale,max,n_threads,outfile);

	// Create a bitmap of the appropriate size.
	bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));
			
	gettimeofday( &begin_time, NULL ); 
	// Compute the Mandelbrot image
	int a; 
	for( a = 0; a < n_threads; a++ )
	{
		if(pthread_create( &td[a], NULL, compute_image,&param_struct[a]))
		{
			perror("Thread could not be created "); 
			exit( EXIT_FAILURE );  
		}
		if(pthread_join( td[a], NULL))
		{
			perror("Error while joining thread "); 
		}
	}

	gettimeofday( &end_time, NULL );  
	time = ( end_time.tv_sec * 1000000 + end_time.tv_usec ) - ( begin_time.tv_sec * 1000000 + begin_time.tv_usec );

	//print the time in the microseconds for generating the curve
	printf("\nTime to execute : %ld microseconds\n", time);
	
	//Redisplay the configuration after image generation 	
	printf("\nMandel Post execution: x=%f y=%f scale=%f Width=%d Height=%d Max=%d threads=%d outfile=%s\n", x_center, y_center, scale, 
						image_width, image_height, max, n_threads, outfile); 
		
	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}

int get_num_bands(int image_height, int n_threads)
{
	return image_height/n_threads; 
}

struct Parameters* create_array_struct(int n_threads, struct Parameters* param_struct_ptr, int image_width, int image_height, float scale, int num_bands)
{

	//set the initial band's minimum width, maximum width,
	//minimum height, maximum height, scale, and number of threads
	param_struct_ptr[0].x_min = 0; 
	param_struct_ptr[0].x_max = image_width; 
	param_struct_ptr[0].n_threads = n_threads; 
	param_struct_ptr[0].y_min = 0; 
	param_struct_ptr[0].y_max = num_bands; 
	param_struct_ptr[0].scale = scale; 
	
	//calculate the number of bands remaining
	int remainder_bands; 
	remainder_bands = 500 % num_bands; 

	//loop through the different bands and set their
	//minimum width, maximum width,
	//minimum height, maximum height, scale, and number of threads
	int f = 1; 
	while (f < n_threads)
	{
		//the maximum values for the previous bands will be the
		//minimum values for the next band.
		param_struct_ptr[f].x_min = 0; 
		param_struct_ptr[f].x_max = image_width; 
		param_struct_ptr[f].scale = scale; 
	    param_struct_ptr[f].n_threads = n_threads; 
		param_struct_ptr[f].y_min = param_struct_ptr[f-1].y_max; 
		param_struct_ptr[f].y_max = param_struct_ptr[f].y_min + num_bands;
		 
		f++; 
	}
	//set the maximum height for the last band.
	param_struct_ptr[f-1].y_max = param_struct_ptr[f-1].y_max + remainder_bands; 
	
	return param_struct_ptr; 
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
parameter list changed to void *paramsarg for using "current_struct"
*/

void *compute_image( void *paramsarg )
{
	struct Parameters *current_struct; 
	current_struct = (struct Parameters *) paramsarg; 	

	int a,b; 
	
	int height = bitmap_height(bm);
	int width = bitmap_width(bm);
	

	//calculating the minimum & maximum width & height
	// based on the center
	double x_min = x_center - scale; 
	double x_max = x_center + scale; 
	double y_min = y_center - scale; 
	double y_max = y_center + scale; 

	// For every pixel in the image...
	
	for(b = current_struct->y_min; b < current_struct->y_max; b++) {
		for(a = current_struct->x_min; a < current_struct->x_max; a++) {
			// Determine the point in x,y space for that pixel.
			double x = x_min + a*(x_max-x_min)/width;
			double y = y_min + b*(y_max-y_min)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			bitmap_set(bm,a,b,iters);
		}
	}

	return NULL; 
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int b, int max )
{
	int gray = 255*b/max;
	return MAKE_RGBA(gray,gray,gray,0);
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}




