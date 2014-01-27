#ifndef TEST_H
#define TEST_H

#define SRC_PAL_ANY     (-1)
#define SRC_PAL_PNG     (0)
#define SRC_PAL_JPEG    (1)
#define SRC_PAL_MJPEG   (2)
#define SRC_PAL_S561    (3)
#define SRC_PAL_RGB32   (4)
#define SRC_PAL_BGR32   (5)
#define SRC_PAL_RGB24   (6)
#define SRC_PAL_BGR24   (7)
#define SRC_PAL_YUYV    (8)
#define SRC_PAL_UYVY    (9)
#define SRC_PAL_YUV420P (10)
#define SRC_PAL_NV12MB  (11)
#define SRC_PAL_BAYER   (12)
#define SRC_PAL_SGBRG8  (13)
#define SRC_PAL_SGRBG8  (14)
#define SRC_PAL_RGB565  (15)
#define SRC_PAL_RGB555  (16)
#define SRC_PAL_Y16     (17)
#define SRC_PAL_GREY    (18)

// HACK, das gehört ins Header-File
typedef uint32_t avgbmp_t;
#define MAX_FRAMES (UINT32_MAX >> 8)

typedef struct {
        char    *name;
} src_palette_t;

extern src_palette_t src_palette[];

typedef struct {
	uint32_t width;
	uint32_t height;
	double *data;
} hc_dpix_t;


typedef struct {
	
	/* Source Options */
	char *source;
	uint8_t type;
	
	void *state;
	
	/* Last captured image */
	uint32_t length;
	void *img;
	
	/* Input Options */
	char    *input;
	uint32_t timeout;
	char     use_read;
	
	/* List Options */
	uint8_t list;
	
	/* Image Options */
	int palette;
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	
	// src_option_t **option;
	
	/* For calculating capture FPS */
	uint32_t captured_frames;
	struct timeval tv_first;
	struct timeval tv_last;
	
} src_t;

#define CLIP(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

extern uint32_t complex_save( fftw_complex *data, uint32_t real, uint32_t w, uint32_t h,char *filename);
extern uint32_t double_save( double *data, uint32_t w, uint32_t h,char *filename);

#endif