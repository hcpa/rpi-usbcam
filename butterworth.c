#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <time.h>
#include "test.h"
#include "log.h"
#include "fft.h"
#include "butterworth.h"

static hc_dpix_t *hcCreateButterworthHiPassFilter( uint32_t w, uint32_t h, double radius, double order )
{
	int32_t	   hw, hh, i, j;
	double     dist;
	double     *data;
	hc_dpix_t  *dpix;
	
	if ( NULL == (dpix = calloc(sizeof(hc_dpix_t),1)) )
	{
		ERROR("out of memory");
		return NULL;
	}
	
	dpix->width = w/2+1;
	dpix->height = h;
	if( NULL == (dpix->data = calloc(sizeof(double),(w/2+1)*h)) )
	{
		ERROR("out of memory");
		return NULL;
	}
	
	/* Create the filter */
	hw = w / 2, hh = h / 2;
	data = dpix->data;
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w / 2 + 1; j++)
		{
			dist = sqrt(pow(j - hw, 2) + pow(i - hh, 2));
			(*data) = 1 / (1 + pow( dist / radius, 2 * order)); // LO-PASS
//			(*data) = 1 / (1 + pow( radius / dist, 2 * order)); // HI-PASS
			data++;
		}
	}
	
	return dpix;
	
}

// TODO L_THRESH_NEG_TO_BLACK
static hc_dpix_t *hcApplyFilter( hc_dpix_t *src, hc_dpix_t *filter )
{
	uint32_t	 w, h;
	uint32_t     i, j, k;
	fftw_complex *output;
	double     	 *data;
	hc_dpix_t	*dpixd;
	
	if (!src && !filter)
	{
        ERROR("src or filter not defined");
		return(NULL);
	}

	w=src->width;
	h=src->height;
	if (filter->width > w / 2 + 1 || filter->height > h)
	{
		ERROR("filter is smaller than source");
		return(NULL);
	}
	
	/* Calculate the DFT of pixs */
	// TODO was with L_WITH_SHIFTING, does it make a difference?
	if (NULL == (output = dpixDFT( src )))
	{
        ERROR("src DFT not computed");
		return(NULL);
	}
	
	/* Filter the DFT results */
	data = filter->data;
	for (i = 0, k = 0; i < h; i++)
	{
		for (j = 0; j < w / 2 + 1; j++, k++)
		{
			output[k] *= (*data);
			data++;
		}
	}
	
	/* Compute the inverse of the DFT */
	dpixd = dpixInverseDFT(output, w, h );
	
	/* Release the allocated resources */
	fftw_free(output);
	
	return dpixd;
}


hc_dpix_t *hcButterworthHiPassFilter( hc_dpix_t *src, double radius, uint32_t order)
{
	hc_dpix_t *filter;
	hc_dpix_t *result;
	
	filter = hcCreateButterworthHiPassFilter( src->width, src->height, radius, order);
	
	double_save(filter->data, filter->width, filter->height, "filter.jpg");
	
	result = hcApplyFilter( src, filter );
	
	free(filter->data);
	free(filter);
	
	return result;
}
