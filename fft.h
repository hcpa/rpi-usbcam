#ifndef FFT_H
#define FFT_H

int32_t
pixPhaseCorrelation(hc_dpix_t       *pixr,
					hc_dpix_t       *pixs,
					double 			*ppeak,
					int32_t   		*pxloc,
					int32_t   		*pyloc);


hc_dpix_t *dpixInverseDFT(fftw_complex *dft, int32_t w, int32_t h);
fftw_complex *dpixDFT(hc_dpix_t *dpix);

#endif