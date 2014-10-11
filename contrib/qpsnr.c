/*
 * Adapted from:
 * qpsnr (C) 2010 E. Oriani, ema <AT> fastwebnet <DOT> it
 * http://qpsnr.youlink.org
 *
 * NB: No QA of qpsnr algorithms
 *
 * qpsnr is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * qpsnr is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with qpsnr.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <math.h>


double compute_psnr(const uint8_t *ref, const uint8_t *cmp, unsigned pixels)
{
  double mse = 0.0;
  unsigned i;
  
  for (i = 0; i < pixels; ++i) {
    const int diff = ref[i]-cmp[i];
    mse += (diff*diff);
  }

  mse /= (double) pixels;

  if (mse == 0.0) {
    mse = 1e-10;
  }

  return 10.0*log10(65025.0/mse);
}

/**
 * Compute SSIM for 8x8 blocks
 *
 * Dissection of the algorithm:
 * http://en.wikipedia.org/w/index.php?title=Structural_similarity&oldid=615214687
 */
double compute_ssim_8x8(const uint8_t *ref, const uint8_t *cmp, unsigned w, unsigned h)
{
  unsigned x, y, i, j;
  double avg = 0;

  for (y = 0; y < h; y += 8) {
    for (x = 0; x < w; x += 8) {
      const unsigned offset = y*w + x;
      unsigned ref_acc = 0;
      unsigned ref_acc_2 = 0;
      unsigned cmp_acc = 0;
      unsigned cmp_acc_2 = 0;
      unsigned ref_cmp_acc = 0;

      for (j = 0; j < 8; j++) {
	for (i = 0; i < 8; i++) {
	  const unsigned c_ref = ref[offset + j*w + i];
	  const unsigned c_cmp = cmp[offset + j*w + i];
	  ref_acc += c_ref;
	  ref_acc_2 += c_ref*c_ref;
	  cmp_acc += c_cmp;
	  cmp_acc_2 += c_cmp*c_cmp;
	  ref_cmp_acc += c_ref*c_cmp;
	}
      }

      const double n_samples = 64;
      double ref_avg = ref_acc/n_samples,
	ref_var = ref_acc_2/n_samples - ref_avg*ref_avg,
	cmp_avg = cmp_acc/n_samples,
	cmp_var = cmp_acc_2/n_samples - cmp_avg*cmp_avg,
	ref_cmp_cov = ref_cmp_acc/n_samples - (ref_avg*cmp_avg);
      const double c1 = 6.5025;
      const double c2 = 58.5225;
      double ssim_num = (2.0*ref_avg*cmp_avg + c1)*(2.0*ref_cmp_cov + c2);
      double ssim_den = (ref_avg*ref_avg + cmp_avg*cmp_avg + c1)*(ref_var + cmp_var + c2);

      avg += ssim_num/ssim_den;
    }
  }

  avg /= (w*h)/64;
  return avg;
}
