#ifndef QPSNR_H
#define QPSNR_H

double compute_psnr(const uint8_t *ref, const uint8_t *cmp, unsigned pixels);
double compute_ssim_8x8(const uint8_t *ref, const uint8_t *cmp, unsigned w, unsigned h);

#endif
