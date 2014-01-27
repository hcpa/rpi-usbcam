#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <gd.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include "test.h"
#include "log.h"
#include "parse.h"
#include "src_v4l2.h"
#include "fft.h"
#include "butterworth.h"

#define FORMAT_JPEG (0)
#define FORMAT_PNG  (1)

enum hc_options {
	OPT_VERSION = 128,
	OPT_JPEG,
	OPT_PNG,
	OPT_SAVE,
	OPT_DUMPFRAME,
	OPT_FPS,
};

src_palette_t src_palette[] = {
	{ "PNG" },
	{ "JPEG" },
	{ "MJPEG" },
	{ "S561" },
	{ "RGB32" },
	{ "BGR32" },
	{ "RGB24" },
	{ "BGR24" },
	{ "YUYV" },
	{ "UYVY" },
	{ "YUV420P" },
	{ "NV12MB" },
	{ "BAYER" },
	{ "SGBRG8" },
	{ "SGRBG8" },
	{ "RGB565" },
	{ "RGB555" },
	{ "Y16" },
	{ "GREY" },
	{ NULL }
};


typedef struct {
	
	/* List of options. */
	char *opts;
	const struct option *long_opts;
	
	/* When reading from the command line. */
	int opt_index;
	
} hc_getopt_t;


typedef struct {
	
	/* General options. */
	// unsigned long loop;
	// signed long offset;
	// unsigned char background;
	// char *pidfile;
	// char *logfile;
	// char gmt;
	
	/* Capture start time. */
	time_t start;
	
	/* Device options. */
	char *device;
	// char *input;
	// unsigned char tuner;
	// unsigned long frequency;
	char use_read;
	uint8_t list;
	
	/* Image capture options. */
	unsigned int width;
	unsigned int height;
	unsigned int frames;
	unsigned int fps;
	unsigned int skipframes;
	int palette;
	// src_option_t **option;
	char *dumpframe;
	
	/* Job queue. */
	// uint8_t jobs;
	// fswebcam_job_t **job;
	
	/* Banner options. */
	// char banner;
	// uint32_t bg_colour;
	// uint32_t bl_colour;
	// uint32_t fg_colour;
	// char *title;
	// char *subtitle;
	// char *timestamp;
	// char *info;
	// char *font;
	// int fontsize;
	// char shadow;
	
	/* Overlay options. */
	// char *underlay;
	// char *overlay;
	
	/* Output options. */
	char *filename;
	char format;
	char compression;
	
} hc_config_t;

int verify_jpeg_dht(uint8_t *src,  uint32_t lsrc,
                    uint8_t **dst, uint32_t *ldst)
{
	/* This function is based on a patch provided by Scott J. Bertin. */
	
	static unsigned char dht[] =
	{
		0xff, 0xc4, 0x01, 0xa2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x09, 0x0a, 0x0b, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02,
		0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
		0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31,
		0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32,
		0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52,
		0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
		0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
		0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45,
		0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57,
		0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
		0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83,
		0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94,
		0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
		0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
		0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
		0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
		0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
		0xf9, 0xfa, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
		0x0b, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
		0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01,
		0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
		0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14,
		0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
		0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25,
		0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a,
		0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46,
		0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
		0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
		0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83,
		0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94,
		0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
		0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
		0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
		0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
		0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
	};
	uint8_t *p, *i = NULL;
	
	/* By default we simply return the source image. */
	*dst = src;
	*ldst = lsrc;
	
	/* Scan for an existing DHT segment or the first SOS segment. */
	for(p = src + 2; p - src < lsrc - 3 && i == NULL; )
	{
		if(*(p++) != 0xFF) continue;
		
		if(*p == 0xD9) break;           /* JPEG_EOI */
		if(*p == 0xC4) return(0);       /* JPEG_DHT */
		if(*p == 0xDA && !i) i = p - 1; /* JPEG_SOS */
		
		/* Move to next segment. */
		p += (p[1] << 8) + p[2];
	}
	
	/* If no SOS was found, insert the DHT directly after the SOI. */
	if(i == NULL) i = src + 2;
	
	DEBUG("Inserting DHT segment into JPEG frame.");
	
	*ldst = lsrc + sizeof(dht);
	*dst  = malloc(*ldst);
	if(!*dst)
	{
		ERROR("Out of memory.");
		return(-1);
	}
	
	/* Copy the JPEG data, inserting the DHT segment. */
	memcpy((p  = *dst), src, i - src);
	memcpy((p += i - src), dht, sizeof(dht));
	memcpy((p += sizeof(dht)), i, lsrc - (i - src));
	
	return(1);
}

int hc_add_image_jpeg(src_t *src, avgbmp_t *abitmap)
{
	uint32_t x, y, hlength;
	uint8_t *himg = NULL;
	gdImage *im;
	int i;
	
	/* MJPEG data may lack the DHT segment required for decoding... */
	i = verify_jpeg_dht(src->img, src->length, &himg, &hlength);
	
	im = gdImageCreateFromJpegPtr(hlength, himg);
	if(i == 1) free(himg);
	
	if(!im) return(-1);
	
	for(y = 0; y < src->height; y++)
		for(x = 0; x < src->width; x++)
		{
			int c = gdImageGetPixel(im, x, y);
			
			*(abitmap++) += (c & 0xFF0000) >> 16;
			*(abitmap++) += (c & 0x00FF00) >> 8;
			*(abitmap++) += (c & 0x0000FF);
		}
	
	gdImageDestroy(im);
	
	return(0);
}

int hc_add_image_yuyv(src_t *src, avgbmp_t *abitmap)
{
	uint8_t *ptr;
	uint32_t x, y, z;
	
	if(src->length < (src->width * src->height * 2)) return(-1);
	
	/* YUYV and UYVY are very similar and so  *
	 * are both handled by this one function. */
	
	ptr = (uint8_t *) src->img;
	z = 0;
	
	for(y = 0; y < src->height; y++)
	{
		for(x = 0; x < src->width; x++)
		{
			int r, g, b;
			int y;
			
			if(src->palette == SRC_PAL_UYVY)
			{
				if(!z) y = ptr[1];
				else   y = ptr[3];
				
			}
			else /* SRC_PAL_YUYV */
			{
				if(!z) y = ptr[0];
				else   y = ptr[2];	
			}
			
			r = y;
			g = y;
			b = y;
			
			*(abitmap++) += CLIP(r, 0x00, 0xFF);
			*(abitmap++) += CLIP(g, 0x00, 0xFF);
			*(abitmap++) += CLIP(b, 0x00, 0xFF);
			
			if(z++)
			{
				z = 0;
				ptr += 4;
			}
		}
	}
	
        return(0);
}


// int hc_add_image_yuyv(src_t *src, avgbmp_t *abitmap)
// {
// 	uint8_t *ptr;
// 	uint32_t x, y, z;
// 	
// 	if(src->length < (src->width * src->height * 2)) return(-1);
// 	
// 	/* YUYV and UYVY are very similar and so  *
// 	 * are both handled by this one function. */
// 	
// 	ptr = (uint8_t *) src->img;
// 	z = 0;
// 	
// 	for(y = 0; y < src->height; y++)
// 	{
// 		for(x = 0; x < src->width; x++)
// 		{
// 			int r, g, b;
// 			int y, u, v;
// 			
// 			if(src->palette == SRC_PAL_UYVY)
// 			{
// 				if(!z) y = ptr[1] << 8;
// 				else   y = ptr[3] << 8;
// 				
// 				u = ptr[0] - 128;
// 				v = ptr[2] - 128;
// 			}
// 			else /* SRC_PAL_YUYV */
// 			{
// 				if(!z) y = ptr[0] << 8;
// 				else   y = ptr[2] << 8;
// 				
// 				u = ptr[1] - 128;
// 				v = ptr[3] - 128;
// 			}
// 			
// 			r = (y + (359 * v)) >> 8;
// 			g = (y - (88 * u) - (183 * v)) >> 8;
// 			b = (y + (454 * u)) >> 8;
// 			
// 			*(abitmap++) += CLIP(r, 0x00, 0xFF);
// 			*(abitmap++) += CLIP(g, 0x00, 0xFF);
// 			*(abitmap++) += CLIP(b, 0x00, 0xFF);
// 			
// 			if(z++)
// 			{
// 				z = 0;
// 				ptr += 4;
// 			}
// 		}
// 	}
// 	
//         return(0);
// }


int hc_find_palette(char *name)
{
	int i;
	
	/* Scan through the palette table until a match is found. */
	for(i = 0; src_palette[i].name != NULL; i++)
	{
		/* If a match was found, return the index. */
		if(!strcasecmp(src_palette[i].name, name)) return(i);
	}
	
	/* No match was found. */
	ERROR("Unrecognised palette format \"%s\". Supported formats:", name);
	
	for(i = 0; src_palette[i].name != NULL; i++)
		ERROR("%s", src_palette[i].name);
	
	return(-1);
}


int hc_usage()
{
	printf("Usage: test [<options>] <filename> [[<options>] <filename> ... ]\n"
	       "\n"
	       " Options:\n"
	       "\n"
	       " -?, --help                   Display this help page and exit.\n"
	       " -q, --quiet                  Hides all messages except for errors.\n"
	       " -v, --verbose                Displays extra messages while capturing\n"
	       "     --version                Displays the version and exits.\n"
	       " -d, --device <name>          Sets the source to use.\n"
	       " -p, --palette <name>         Selects the palette format to use.\n"
	       " -r, --resolution <size>      Sets the capture resolution.\n"
	       "     --fps <framerate>        Sets the capture frame rate.\n"
	       " -F, --frames <number>        Sets the number of frames to capture.\n"
	       " -S, --skip <number>          Sets the number of frames to skip.\n"
	       "     --dumpframe <filename>   Dump a raw frame to file.\n"
	       "     --jpeg <factor>          Outputs a JPEG image. (-1, 0 - 95)\n"
	       "     --png <factor>           Outputs a PNG image. (-1, 0 - 10)\n"
	       // "     --save <filename>        Save image to file.\n"
	       "\n");
	
	return(0);
}


int hc_getopt(hc_getopt_t *s, int argc, char *argv[])
{
	int c;
	
	/* Read from the command line. */
	c = getopt_long(argc, argv, s->opts, s->long_opts, &s->opt_index);
	
	return(c);
}



int hc_getopts(hc_config_t *config, int argc, char *argv[])
{
	int c;
	hc_getopt_t s;
	static struct option long_opts[] =
	{
		{"help",            no_argument,       0, '?'},
		{"config",          required_argument, 0, 'c'},
		{"quiet",           no_argument,       0, 'q'},
		{"verbose",         no_argument,       0, 'v'},
		{"version",         no_argument,       0, OPT_VERSION},
		{"device",          required_argument, 0, 'd'},
		{"resolution",      required_argument, 0, 'r'},
		{"fps",	            required_argument, 0, OPT_FPS},
		{"frames",          required_argument, 0, 'F'},
		{"skip",            required_argument, 0, 'S'},
		{"palette",         required_argument, 0, 'p'},
		{"dumpframe",       required_argument, 0, OPT_DUMPFRAME},
		{"read",            no_argument,       0, 'R'},
		{"jpeg",            required_argument, 0, OPT_JPEG},
		{"png",             required_argument, 0, OPT_PNG},
		// {"save",            required_argument, 0, OPT_SAVE},
		{0, 0, 0, 0}
	};
	char *opts = "-qvd:i:r:F:S:p:R";
	
	s.opts      = opts;
	s.long_opts = long_opts;
	s.opt_index = 0;
	
	/* Set the defaults. */
	config->start = 0;
	config->device = strdup("/dev/video0");
	config->use_read = 0;
	config->list = 0;
	config->width = 384;
	config->height = 288;
	config->fps = 0;
	config->frames = 1;
	config->skipframes = 0;
	config->palette = SRC_PAL_ANY;
	config->dumpframe = NULL;
	
	/* Don't report errors. */
	opterr = 0;
	
	/* Reset getopt to ensure parsing begins at the first argument. */
	optind = 0;
	
	/* Parse the command line and any config files. */
	while((c = hc_getopt(&s, argc, argv)) != -1)
	{
		switch(c)
		{
		case '?': hc_usage(); /* Command line error. */
		case '!': return(-1);   /* Conf file error. */
		case OPT_VERSION:
			fprintf(stderr, "fswebcam %s\n", PROGRAM_VERSION);
			return(-1);
		case 'q':
			log_quiet(-1);
			log_verbose(0);
			break;
		case 'v':
			log_quiet(0);
			log_verbose(-1);
			break;
		case 'd':
			if(config->device) free(config->device);
			config->device = strdup(optarg);
			break;
		case 'r':
	 		config->width  = argtol(optarg, "x ", 0, 0, 10);
			config->height = argtol(optarg, "x ", 1, 0, 10);
			break;
		case OPT_FPS:
			config->fps = atoi(optarg);
			break;
		case 'F':
			config->frames = atoi(optarg);
			break;
		case 'S':
			config->skipframes = atoi(optarg);
			break;
		case 'p':
			config->palette = hc_find_palette(optarg);
			if(config->palette == -1) return(-1);
			break;
		case 'R':
			config->use_read = -1;
			break;
		case OPT_DUMPFRAME:
			free(config->dumpframe);
			config->dumpframe = strdup(optarg);
			break;
		default:
			/* All other options are added to the job queue. */
			WARN("unknown argument %c",c);
			break;
		}
	}
	
	/* Do a sanity check on the options. */
	if(config->width < 1)           config->width = 1;
	if(config->height < 1)          config->height = 1;
	if(config->frames < 1)          config->frames = 1;
	if(config->frames > MAX_FRAMES)
	{
		WARN("Requested %u frames, maximum is %u. Using that.",
		   config->frames, MAX_FRAMES);
		
		config->frames = MAX_FRAMES;
	}
	
	return(0);
}

uint32_t complex_save( fftw_complex *data, uint32_t real, uint32_t w, uint32_t h, char *name)
{
	FILE *f;
	gdImage *image;
	uint32_t i, j, g, color;
	fftw_complex *dat;
	double d, max;
	
	if(!name)
		return(-1);
	
	f = fopen(name, "wb");
	
	if(!f)
	{
		ERROR("Error opening file for output: %s", name);
		ERROR("fopen: %s", strerror(errno));
		return(-1);
	}
	
	image = gdImageCreateTrueColor(w,h);
	if(!image)
	{
		ERROR("Cannot create image");
		fclose(f);
		return(-1);
	}

	max=0;
	dat=data;
	for(j=0;j<h;j++)
		for(i=0;i<w;i++)
		{
			if(real)
				d = creal(*dat);
			else
				d = cimag(*dat);
			dat++;
			if(d>max) max=d;
		}		
	
	
	dat = data;
	for(j=0; j<h; j++){
		for(i=0; i<w; i++)
		{
			if(real)
				d = creal(*dat);
			else
				d = cimag(*dat);
			dat++;
			g = 0xff * d/max;
			color = g + (g<<8) + (g<<16);
			color &= 0xffffff; 
			gdImageSetPixel(image,i,j,color);
		}
	}
	
	MSG("Writing JPEG image to '%s'.", name);
	gdImageJpeg(image, f, 255);

	gdImageDestroy(image);
	return(0);
}


uint32_t double_save( double *data, uint32_t w, uint32_t h, char *name )
{
	FILE *f;
	gdImage *image;
	uint32_t i, j, g, color;
	double *dat, d, max;
	
	if(!name)
		return(-1);
	
	f = fopen(name, "wb");
	
	if(!f)
	{
		ERROR("Error opening file for output: %s", name);
		ERROR("fopen: %s", strerror(errno));
		return(-1);
	}
	
	image = gdImageCreateTrueColor(w,h);
	if(!image)
	{
		ERROR("Cannot create image");
		fclose(f);
		return(-1);
	}

	max=0;
	dat=data;
	for(j=0;j<h;j++)
		for(i=0;i<w;i++)
		{
			d = *(dat++);
			if(d>max) max=d;
		}		
	
	
	dat = data;
	for(j=0; j<h; j++){
		for(i=0; i<w; i++)
		{
			d = *(dat++);
			g = 0xff * d/max;
			color = g + (g<<8) + (g<<16);
			color &= 0xffffff; 
			gdImageSetPixel(image,i,j,color);
		}
	}
	
	MSG("Writing JPEG image to '%s'.", name);
	gdImageJpeg(image, f, 255);

	gdImageDestroy(image);
	return(0);
}

int hc_output(hc_config_t *config, char *name, gdImage *image)
{
	FILE *f;
	
	if(!name) return(-1);

	/* Write to a file if a filename was given, otherwise stdout. */
	f = fopen(name, "wb");
	
	if(!f)
	{
		ERROR("Error opening file for output: %s", name);
		ERROR("fopen: %s", strerror(errno));
		return(-1);
	}
	
	/* Write the compressed image. */
	switch(config->format)
	{
	case FORMAT_JPEG:
		MSG("Writing JPEG image to '%s'.", name);
		gdImageJpeg(image, f, config->compression);
		break;
	case FORMAT_PNG:
		MSG("Writing PNG image to '%s'.", name);
		gdImagePngEx(image, f, config->compression);
		break;
	}
	
	if(f != stdout) fclose(f);
	
	return(0);
}



int hc_grab(hc_config_t *config, hc_dpix_t *dpix, char *filename)
{
	uint32_t frame;
	uint32_t x,y;
	avgbmp_t *abitmap,*pbitmap;
	gdImage *original; 
	src_t src;
	

	/* Set source options... */
	memset(&src, 0, sizeof(src));
	src.timeout    = 10; /* seconds */
	src.palette    = config->palette;
	src.width      = config->width;
	src.height     = config->height;
	src.source		= "/dev/video0";
	
	HEAD("--- Opening device...");
	
	if(src_v4l2_open(&src) == -1) return(-1);
	
	/* The source may have adjusted the width and height we passed
	 * to it. Update the main config to match. */
	config->width  = src.width;
	config->height = src.height;
	
	/* Allocate memory for the average bitmap buffer. */
	abitmap = calloc(config->width * config->height * 3, sizeof(avgbmp_t));
	if(!abitmap)
	{
		ERROR("Out of memory.");
		return(-1);
	}
	
	if(config->frames == 1) HEAD("--- Capturing frame...");
	else HEAD("--- Capturing %i frames...", config->frames);
	
	if(config->skipframes == 1) MSG("Skipping frame...");
	else if(config->skipframes > 1) MSG("Skipping %i frames...", config->skipframes);
	
	/* Grab (and do nothing with) the skipped frames. */
	for(frame = 0; frame < config->skipframes; frame++)
		if(src_v4l2_grab(&src) == -1) break;
	
	/* If frames where skipped, inform when normal capture begins. */
	if(config->skipframes) MSG("Capturing %i frames...", config->frames);
	
	/* Grab the requested number of frames. */
	for(frame = 0; frame < config->frames; frame++)
	{
		if(src_v4l2_grab(&src) == -1) break;
		
		if(!frame && config->dumpframe)
		{
			/* Dump the raw data from the first frame to file. */
			FILE *f;
			
			MSG("Dumping raw frame to '%s'...", config->dumpframe);
			
			f = fopen(config->dumpframe, "wb");
			if(!f) ERROR("fopen: %s", strerror(errno));
			else
			{
				fwrite(src.img, 1, src.length, f);
				fclose(f);
			}
		}
		
		/* Add frame to the average bitmap. */
		switch(src.palette)
		{
		case SRC_PAL_JPEG:
		case SRC_PAL_MJPEG:
			hc_add_image_jpeg(&src, abitmap);
			break;
			case SRC_PAL_YUYV:
		case SRC_PAL_UYVY:
			hc_add_image_yuyv(&src, abitmap);
			break;
		}
	}
	
	/* We are now finished with the capture card. */
	src_v4l2_close(&src);
	
	/* Fail if no frames where captured. */
	if(!frame)
	{
		ERROR("No frames captured.");
		free(abitmap);
		return(-1);
	}
	
	HEAD("--- Processing captured image...");
	
	// Copy image data to double-array size width*2 by height*2 , including zero-padding
	if( dpix )
	{
		// padding
		// dpix->width = src.width*2;
		// dpix->height = src.height*2;
		dpix->width = src.width;
		dpix->height = src.height;
		dpix->data = malloc(sizeof(double) * dpix->width * dpix->height);
		memset(dpix->data,sizeof(double) * dpix->width * dpix->height,0);
		if( !dpix->data )
		{
			ERROR("Out of memory.");
			free(abitmap);
			return(-1);			
		}
		
		double *dat = dpix->data;
		pbitmap = abitmap;
		for(y = 0; y < config->height; y++)
		{
			for(x = 0; x < config->width; x++)
			{
				double gray;
				
				// TODO
				// eventuell auf einen Frame normalisieren? (r/config->frames + g/config->frames + b/config->frames) / 3
				// muss ich schauen
				gray = *(pbitmap++);
				gray += *(pbitmap++);
				gray += *(pbitmap++);
				*(dat++) = gray/3.0;
			}
			// right padding
			// dat += config->width;
		}
		
		
	}
	
	
	config->format       = FORMAT_JPEG;
	config->compression  = -1;
	
	original = gdImageCreateTrueColor(config->width, config->height);
	if(!original)
	{
		ERROR("Out of memory.");
		free(abitmap);
		return(-1);
	}
	
	pbitmap = abitmap;
	for(y = 0; y < config->height; y++)
		for(x = 0; x < config->width; x++)
		{
			int px = x;
			int py = y;
			int colour;
			
			colour  = (*(pbitmap++) / config->frames) << 16;
			colour += (*(pbitmap++) / config->frames) << 8;
			colour += (*(pbitmap++) / config->frames);
			
			gdImageSetPixel(original, px, py, colour);
		}
	
	
	free(abitmap);

	if( filename ) 
		hc_output(config, filename, original);

	gdImageDestroy(original);
	
	return(0);
}



int main(int argc, char *argv[])
{
	hc_config_t *config;
	hc_dpix_t *dpix1, *dpix2, *dpix1_bw, *dpix2_bw;
	int32_t xloc, yloc;
	double peak;
	
	/* Prepare the configuration structure. */
	config = calloc(sizeof(hc_config_t), 1);
	if(!config)
	{
		WARN("Out of memory.");
		return(-1);
	}
	
	dpix1 = calloc(sizeof(hc_dpix_t),1);
	dpix2 = calloc(sizeof(hc_dpix_t),1);
	if(!dpix1 || !dpix2)
	{
		WARN("Out of memory.");
		return(-1);
	}
	
	/* Set defaults and parse the command line. */
	if(hc_getopts(config, argc, argv)) return(-1);
	
	/* Capture the image(s). */

	hc_grab(config, dpix1, "bild1.jpg");
	
	MSG("Sleeping for 3 seconds");
	sleep(3);

	hc_grab(config, dpix2, "bild2.jpg");

	double_save(dpix1->data, dpix1->width, dpix1->height, "dpix1.jpg");
	double_save(dpix2->data, dpix2->width, dpix2->height, "dpix2.jpg");
	
	dpix1_bw = hcButterworthHiPassFilter( dpix1, 200, 4 );
	dpix2_bw = hcButterworthHiPassFilter( dpix2, 200, 4 );
	
	double_save( dpix1_bw->data, dpix1_bw->width, dpix1_bw->height, "dpix1_bw.jpg" );
	double_save( dpix2_bw->data, dpix2_bw->width, dpix2_bw->height, "dpix2_bw.jpg" );


	if( pixPhaseCorrelation(dpix1_bw, dpix2_bw, &peak, &xloc, &yloc) )
	{
		ERROR("Phasenkorrelation ist völlig überraschend schief gegangen");
		return(-1);
	}
	MSG("peak: %f, x: %d, y:%d", peak, xloc, yloc );

	free(dpix1_bw->data);
	free(dpix1_bw);
	
	free(dpix2_bw->data);
	free(dpix2_bw);

	/* Free all used memory. */
	// fswc_free_config(config);
	free(dpix1->data);
	free(dpix1);
	free(dpix2->data);
	free(dpix2);
	
	free(config);
	
	return(0);
}

