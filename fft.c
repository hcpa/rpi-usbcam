#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include "test.h"
#include "log.h"


/*!
 *  dpixGetMax()
 *
 *      Input:  dpix
 *              &maxval (<optional return> max value)
 *              &xmaxloc (<optional return> x location of max)
 *              &ymaxloc (<optional return> y location of max)
 *      Return: 0 if OK; 1 on error
 */
int32_t
dpixGetMax(hc_dpix_t	*dpix,
           double		*pmaxval,
           int32_t    	*pxmaxloc,
           int32_t    	*pymaxloc)
{
	int32_t     i, j, w, h, xmaxloc, ymaxloc;
	double  *data;
	double   maxval;
	
    if (!pmaxval && !pxmaxloc && !pymaxloc)
	{
        ERROR("nothing to do");
		return(-1);
	}
    if (pmaxval) *pmaxval = 0.0;
    if (pxmaxloc) *pxmaxloc = 0;
    if (pymaxloc) *pymaxloc = 0;
    if (!dpix)
	{
        ERROR("dpix not defined");
		return(-1);
	}
	
    maxval = -1.0e40;
    xmaxloc = 0;
    ymaxloc = 0;
    w = dpix->width;
	h = dpix->height;
    data = dpix->data;
    for (i = 0; i < h; i++)
	{
        for (j = 0; j < w; j++)
		{
            if( (*data) > maxval )
			{
                maxval = (*data);
                xmaxloc = j;
                ymaxloc = i;
            }
			data++;
        }
    }
	
    if (pmaxval) *pmaxval = maxval;
    if (pxmaxloc) *pxmaxloc = xmaxloc;
    if (pymaxloc) *pymaxloc = ymaxloc;
    return 0;
}


/*--------------------------------------------------------------------*
 *                 DPix normalization and shifting                    *
 *--------------------------------------------------------------------*/
/*!
 *  dpixNormalize()
 *
 *      Input:  dpixd  (<optional>; this can be null, equal to dpixs,
 *                     or different from dpixs)
 *              dpixs
 *      Return: dpixd, or null on error
 *
 *  Notes:
 *      (1) Normalize dpixs by dividing each value by the size of the
 *          image. This is to compensate for the FFTW library returning
 *          unnormalized DFT values.
 *      (2) There are 3 cases:
 *           (a) dpixd == null,   ~src --> new dpixd
 *           (b) dpixd == dpixs,  ~src --> src  (in-place)
 *           (c) dpixd != dpixs,  ~src --> input dpixd
 *      (3) For clarity, if the case is known, use these patterns:
 *           (a) dpixd = dpixNormalize(NULL, dpixs);
 *           (b) dpixNormalize(dpixs, dpixs);
 *           (c) dpixNormalize(dpixd, dpixs);
 */
int32_t
dpixNormalize(hc_dpix_t *dpixs)
{
	int32_t     i, j;
	double  *data, n;
	
	
    if (!dpixs)
	{
		ERROR("dpixs not defined");
		return(-1);
	}
	
	
	n = dpixs->width * dpixs->height;
	data = dpixs->data;
	
	for (i = 0; i < dpixs->height; i++)
		for (j = 0; j < dpixs->width; j++)
		{
			*data = (*data)/n;
			data++;
		}
	
	return 0;
}


/*!
 *  dpixInverseDFT()
 *
 *      Input:  dft
 *              w, h (image size)
 *      Return: dpix (unnormalized), or null on error
 */
hc_dpix_t *
dpixInverseDFT(fftw_complex *dft,
			   int32_t       w,
			   int32_t       h)
{
	hc_dpix_t	*dpix;
	fftw_plan	plan;
	
    if (!dft)
    {
    	ERROR("dft not defined");
		return NULL;
	}
	
	if( NULL == (dpix=calloc(sizeof(hc_dpix_t),1)) )
	{
		ERROR("out of memory");
		return NULL;
	}
	
	dpix->width  = w;
	dpix->height = h;
	
	if( NULL == (dpix->data = malloc(sizeof(double) * w * h)) )
	{
		ERROR("out of memory");
		free(dpix);
		return NULL;
	}
	
	/* Compute the inverse DFT, storing the results into DPix */
	plan = fftw_plan_dft_c2r_2d(h, w, dft, dpix->data, FFTW_ESTIMATE);
	fftw_execute( plan );
	fftw_destroy_plan( plan );
	
	dpixNormalize( dpix );
	
	return dpix;
}


/*--------------------------------------------------------------------*
 *      Low-level implementation of Discrete Fourier Transform        *
 *--------------------------------------------------------------------*/
/*!
 *  pixDFT()
 *
 *      Input:  pix (1 bpp or 8 bpp)
 *              shiftflag (L_NO_SHIFTING or L_WITH_SHIFTING)
 *      Return: complex array, or null on error
 *
 *  Notes:
 *      (1) The complex array returned has size (pixs->h) * (pixs->w / 2 + 1).
 *          This is to save space, given the fact the other half of the
 *          transform can be calculated by the complex conjugate.
 *      (2) By default, the DC of the DFT is in the top left corner (0, 0).
 *          Set @shiftflag to L_WITH_SHIFTING to move the DC to the center.
 *      (3) It is the responsibility of the caller to release the allocated
 *          complex array by invoking fftw_free().
 */

// TODO L_WITH_SHIFTING

fftw_complex *
dpixDFT(hc_dpix_t      *dpix)
{
	fftw_complex  *output;
	fftw_plan      plan;
	
    if (!dpix)
	{
        ERROR("dpix not defined");
		return NULL;
	}

/* Compute the DFT of the DPix */
	output = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * dpix->height * (dpix->width / 2 + 1));
	plan = fftw_plan_dft_r2c_2d(dpix->height, dpix->width, dpix->data, output, FFTW_ESTIMATE);
	fftw_execute(plan);
	
	fftw_destroy_plan(plan);
	
	return output;
}



/*--------------------------------------------------------------------*
 *                         Phase correlation                          *
 *--------------------------------------------------------------------*/
/*!
 *  pixPhaseCorrelation()
 *
 *      Input:  pixr, pixs (double)
 *              &peak (<optional return> phase correlation peak)
 *              &xloc (<optional return> x location of the peak)
 *              &yloc (<optional return> y location of the peak)
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) Phase correlation is a method of image registration,
 *          and uses a fast frequency-domain approach to estimate the
 *          relative translative offset between two similar images.
 *      (2) The reference and input images must have same width
 *          and height.
 *      (3) Determine the location of the highest value (peak) in @r,
 *          defined as the phase cross-correlated 2D array:
 *          r = inverse Fourier (F * G / |F * G|)
 *          where F is the Fourier transform of the reference image and
 *          G is the complex conjugate of the transform of the input.
 *          The peak in @r corresponds to the object translation movement
 *          from the reference to the input images. A value of 1.0  at
 *          location (0, 0) means that the images are identical.
 *      (4) If colormapped, remove to grayscale.
 */
int32_t
pixPhaseCorrelation(hc_dpix_t       *pixr,
					hc_dpix_t       *pixs,
					double 			*ppeak,
					int32_t   		*pxloc,
					int32_t   		*pyloc)
{
	int32_t        	i, j, k;
	double      	cr, ci, r;
	fftw_complex  	*outputr, *outputs, *outputd;
	// PIX           	*pixt1, *pixt2;
	hc_dpix_t     	*dpix;
	
    if (!pixr && !pixs)
	{
    	ERROR("pixr or pixs not defined");
		return(-1);
	}
	if (pixr->width != pixs->width || pixr->height != pixs->height)
    {
		ERROR("pixr and pixs unequal size");
		return(-1);
	}
	if (!ppeak && !pxloc && !pyloc)
	{
        ERROR("nothing to do");
		return(-1);
	}
	
	/* Process the reference image */
    // pixt1 = pixClone(pixr);
	
	/* Process the input image */
    // pixt2 = pixClone(pixs);
    
	/* Calculate the DFT of pixr and pixs */
	if ((outputr = dpixDFT(pixr)) == NULL)
	{
		ERROR("outputr not made");
		return(-1);
	}
	if ((outputs = dpixDFT(pixs)) == NULL) {
		fftw_free(outputr);
		ERROR("outputs not made");
		return(-1);
	}
	outputd = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * pixr->height * (pixr->width / 2 + 1));
	if (outputd == NULL) {
		fftw_free(outputr);
		fftw_free(outputs);
		ERROR("outputd not made");
		return(-1);
	}
	
	/* Calculate the cross-power spectrum */
	for (i = 0, k = 0; i < pixr->height; i++) {
		for (j = 0; j < pixr->width / 2 + 1; j++, k++) {
			cr = creal(outputs[k]) * creal(outputr[k]) - cimag(outputs[k]) * (-cimag(outputr[k]));
			ci = creal(outputs[k]) * (-cimag(outputr[k])) + cimag(outputs[k]) * creal(outputr[k]);
			r = sqrt(pow(cr, 2.) + pow(ci, 2.));
			outputd[k] = (cr / r) + I * (ci / r);
		}
	}
	
	/* Compute the inverse DFT of the cross-power spectrum
	    and find its peak */
	dpix = dpixInverseDFT(outputd, pixr->width, pixr->height);
	
	complex_save(outputs, 1, pixr->width/2+1,pixr->height,"outputr-re.jpg");
	complex_save(outputs, 0, pixr->width/2+1,pixr->height,"outputr-im.jpg");
	complex_save(outputr, 1, pixs->width/2+1,pixs->height,"outputs-re.jpg");
	complex_save(outputr, 0, pixs->width/2+1,pixs->height,"outputs-im.jpg");
	complex_save(outputd, 1, pixr->width/2+1,pixr->height,"outputd-re.jpg");
	complex_save(outputd, 0, pixr->width/2+1,pixr->height,"outputd-im.jpg");
	
	double_save(dpix->data, dpix->width, dpix->height,"revFFTresult.jpg");
	
	dpixGetMax(dpix, ppeak, pxloc, pyloc);
	
	if (*pxloc >= pixr->width / 2)
		*pxloc -= pixr->width;
	if (*pyloc >= pixr->height / 2)
		*pyloc -= pixr->height;
		
	/* Release the allocated resources */
	fftw_free(outputr);
	fftw_free(outputs);
	fftw_free(outputd);
	
	free(dpix->data);
	free(dpix);
	
	return(0);
}
