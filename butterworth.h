#ifndef BUTTERWORTH_H
#define BUTTERWORTH_H

hc_dpix_t *hcButterworthHiPassFilter( hc_dpix_t *src, double radius, uint32_t order);


enum {
    L_CLIP_TO_ZERO = 1,        /* Clip negative values to 0                */
    L_TAKE_ABSVAL = 2,          /* Convert to positive using L_ABS()       */
	
	L_THRESH_NEG_TO_BLACK = 3, /* Set to black all negative values and to
								  white all positive values.
								  The output is a binary image.            */
	L_THRESH_NEG_TO_WHITE = 4  /* Set to black all positive values and to
								  white all negative values.
								  The output is a binary image.            */
};


#endif