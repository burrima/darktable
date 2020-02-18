/*
    This file is part of darktable,
    copyright (c) 2020 Martin Burri.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <cmocka.h>

#include "../util/assert.h"
#include "../util/tracing.h"
#include "../util/testimg.h"

#include "iop/filmicrgb.c"

/*
 * DEFINITIONS
 */

// epsilon for floating point comparison:
#define E 1e-6f

/*
 * MOCKED FUNCTIONS
 */

dt_iop_order_iccprofile_info_t *__wrap_dt_ioppr_get_pipe_work_profile_info(
  struct dt_dev_pixelpipe_t *pipe)
{
  return mock_ptr_type(dt_iop_order_iccprofile_info_t *);
}

/*
 * TEST FUNCTIONS
 */

static void test_full_input_space(void **state)
{
  Testimg *ti_in;
  Testimg *ti_out;
  dt_iop_roi_t roi_in;
  dt_iop_roi_t roi_out;

  TR_STEP("prepare environment (TODO: is there a better way?)");
  struct dt_iop_module_t module;
  init(&module);
  module.commit_params = commit_params;
  struct dt_dev_pixelpipe_t pipe;
  struct dt_dev_pixelpipe_iop_t piece;
  init_pipe(&module, &pipe, &piece);
  piece.pipe = &pipe;
  piece.colors = 4;

  dt_iop_filmicrgb_params_t *params = (dt_iop_filmicrgb_params_t *)module.params;

  TR_STEP("verify that values are in [0.0, 1.0] for all variations of input "
    "settings and all variations of pixel values");
  TR_BUG("white_point_target should not be <10 else values > 1.0 are observed");
  Testimg *testimgs[] = {
    testimg_gen_grey_space(10),
    testimg_gen_rgb_space(5),
    testimg_gen_grey_with_rgb_clipping(10),
    testimg_gen_grey_max_dr(),
    testimg_gen_grey_max_dr_neg()
  };

  for (int ti_idx = 0; ti_idx < 4; ti_idx += 1)
  {
    ti_in = testimgs[ti_idx];
    ti_out = testimg_alloc(ti_in->width, ti_in->height);
    roi_out.width = ti_out->width;
    roi_out.height = ti_out->height;

    TR_DEBUG("Test image: %s", ti_in->name);

    const float N = 3.0f;

    for (params->preserve_color = 0; params->preserve_color < 4; params->preserve_color += 1)
    for (params->balance = -50; params->balance <= 50 + E; params->balance += 100.0/N)
    for (params->saturation = -50; params->saturation <= 50 + E; params->saturation += 100.0/N)
    for (params->contrast = 1.0f; params->contrast < 2.0 + E; params->contrast += 1.0/N)
    for (params->latitude = 5; params->latitude < 45 + E; params->latitude += 40.0/N)
    for (params->output_power = 1; params->output_power < 10 + E; params->output_power += 9.0/N)
    for (params->white_point_target = 10; params->white_point_target < 100 + E; params->white_point_target += 90.0/N)
    for (params->black_point_target = 0; params->black_point_target < 100 + E; params->black_point_target += 100.0/N)
    for (params->grey_point_target = 0.1; params->grey_point_target < 50 + E; params->grey_point_target += 49.9/N)
    for (params->security_factor = -50; params->grey_point_target < 50 + E; params->grey_point_target += 100.0/N)
    for (params->white_point_source = 2; params->white_point_target < 8 + E; params->white_point_target += 6.0/N)
    for (params->black_point_source = -14; params->black_point_source < -3 + E; params->black_point_source += 17.0/N)
    for (params->grey_point_source = 0.1f; params->grey_point_source < 36 + E; params->grey_point_source += 35.9f/N)
    {
      commit_params(&module, params, &pipe, &piece);
      will_return(__wrap_dt_ioppr_get_pipe_work_profile_info, NULL);
      process(&module, &piece, ti_in->pixels, ti_out->pixels, &roi_in, &roi_out);
      for_testimg_pixels_p_xy(ti_out)
      {
        float *p_in = get_pixel(ti_in, x, y);
        for (int c = 0; c < 3; c += 1)
        {
          int is_greater_equal_0 = p[c] >= 0.0f - E;
          int is_smaller_equal_1 = p[c] <= 1.0f - E;

          if (!is_greater_equal_0 || !is_smaller_equal_1)
          {
            TR_DEBUG("Expected output not met!");
            TR_DEBUG("grey_point_source = %f", params->grey_point_source);
            TR_DEBUG("black_point_source = %f", params->black_point_source);
            TR_DEBUG("white_point_source = %f", params->white_point_source);
            TR_DEBUG("security_factor = %f", params->security_factor);
            TR_DEBUG("grey_point_target = %f", params->grey_point_target);
            TR_DEBUG("black_point_target = %f", params->black_point_target);
            TR_DEBUG("white_point_target = %f", params->white_point_target);
            TR_DEBUG("output_power = %f", params->output_power);
            TR_DEBUG("latitude = %f", params->latitude);
            TR_DEBUG("contrast = %f", params->contrast);
            TR_DEBUG("saturation = %f", params->saturation);
            TR_DEBUG("balance = %f", params->balance);
            TR_DEBUG("preserve_color = %i", params->preserve_color);
            TR_DEBUG("{%e, %e, %e} => {%e, %e, %e}",
              p_in[0], p_in[1], p_in[2], p[0], p[1], p[2]);
          }
          assert_true(is_greater_equal_0);
          assert_true(is_smaller_equal_1);
        }
      }
    }
  }
  for (int ti_idx = 0; ti_idx < 4; ti_idx += 1)
  {
    testimg_free(testimgs[ti_idx]);
  }
  testimg_free(ti_out);
}

static float linear_rgb_luminance(const float *const rgb)
{
  return (rgb[0] * 0.279043f + rgb[1] * 0.675345f + rgb[2] * 0.045613f);
}

static void test_process(void **state)
{
  Testimg *ti_in;
  Testimg *ti_out;
  dt_iop_roi_t roi_in;
  dt_iop_roi_t roi_out;

  TR_STEP("prepare environment (TODO: is there a better way?)");
  struct dt_iop_module_t module;
  init(&module);
  module.commit_params = commit_params;
  struct dt_dev_pixelpipe_t pipe;
  struct dt_dev_pixelpipe_iop_t piece;
  init_pipe(&module, &pipe, &piece);
  piece.pipe = &pipe;
  piece.colors = 4;

  dt_iop_filmicrgb_params_t *params = (dt_iop_filmicrgb_params_t *)module.params;

  TR_STEP("veify that s-curve is applied correctly to greyscale and that "
    "pixels remain greyscale");
  ti_in = testimg_gen_grey_space(TESTIMG_STD_WIDTH);
  ti_out = testimg_alloc(ti_in->width, ti_in->height);
  roi_out.width = ti_out->width;
  roi_out.height = ti_out->height;

  commit_params(&module, params, &pipe, &piece);
  will_return(__wrap_dt_ioppr_get_pipe_work_profile_info, NULL);
  process(&module, &piece, ti_in->pixels, ti_out->pixels, &roi_in, &roi_out);

  float prev = 0.0f;
  for_testimg_pixels_p_xy(ti_out)
  {
    // remain greyscale:
    assert_float_equal(p[0], p[1], E);
    assert_float_equal(p[0], p[2], E);
    // values increasing:
    assert_true(p[0] >= prev);
    prev = p[0];
    // values bound to [0.0; 1.0]:
    assert_true(p[0] >= 0.0f - E);
    assert_true(p[0] <= 1.0f + E);
  }

  testimg_free(ti_in);
  testimg_free(ti_out);

  TR_STEP("verify that s-curve is applied correctly to pure colors");
  //ti_in = testimg_gen_three_color_space(TESTIMG_STD_WIDTH);
  ti_in = testimg_gen_grey_with_rgb_clipping(10);
  //ti_in = testimg_gen_grey_space(TESTIMG_STD_WIDTH);
//  ti_in = testimg_alloc(2,1);
//  float *a = get_pixel(ti_in, 0, 0);
//  a[0] = 1.0f;
//  a[1] = 1.0f;
//  a[2] = 1.0f;
//  a = get_pixel(ti_in, 1, 0);
//  a[0] = 2.0f;
//  a[1] = 1.0f;
//  a[2] = 1.0f;
  ti_out = testimg_alloc(ti_in->width, ti_in->height);
  roi_out.width = ti_out->width;
  roi_out.height = ti_out->height;

  params->saturation = 0.0f;
  params->white_point_source = 3.761f;
  params->preserve_color = DT_FILMIC_METHOD_MAX_RGB;
  commit_params(&module, params, &pipe, &piece);

  TR_DEBUG("grey_point_source = %f", params->grey_point_source);
  TR_DEBUG("black_point_source = %f", params->black_point_source);
  TR_DEBUG("white_point_source = %f", params->white_point_source);
  TR_DEBUG("security_factor = %f", params->security_factor);
  TR_DEBUG("grey_point_target = %f", params->grey_point_target);
  TR_DEBUG("black_point_target = %f", params->black_point_target);
  TR_DEBUG("white_point_target = %f", params->white_point_target);
  TR_DEBUG("output_power = %f", params->output_power);
  TR_DEBUG("latitude = %f", params->latitude);
  TR_DEBUG("contrast = %f", params->contrast);
  TR_DEBUG("saturation = %f", params->saturation);
  TR_DEBUG("balance = %f", params->balance);
  TR_DEBUG("preserve_color = %i", params->preserve_color);

  will_return(__wrap_dt_ioppr_get_pipe_work_profile_info, NULL);
  params->preserve_color = DT_FILMIC_METHOD_NONE;
  printf("preserve_color = NONE\n");
  commit_params(&module, params, &pipe, &piece);
  process(&module, &piece, ti_in->pixels, ti_out->pixels, &roi_in, &roi_out);
  for_testimg_pixels_p_yx(ti_out)
  {
      float *p_in = get_pixel(ti_in, x, y);
      // used in dt when rec2020 is configured: 0.279043, 0.675345, 0.045613
      // float lum = p[0] * 0.2126 + p[1] * 0.7152 + p[2] * 0.0722;
      float lum_in = linear_rgb_luminance(p_in);
      float lum = linear_rgb_luminance(p);
      TR_DEBUG("{%e, %e, %e} lum=%e => {%e, %e, %e} lum=%e",
        p_in[0], p_in[1], p_in[2], lum_in, p[0], p[1], p[2], lum);
    // TODO: assert
  }

  will_return(__wrap_dt_ioppr_get_pipe_work_profile_info, NULL);
  params->preserve_color = DT_FILMIC_METHOD_LUMINANCE;
  printf("preserve_color = LUMINANCE\n");
  commit_params(&module, params, &pipe, &piece);
  process(&module, &piece, ti_in->pixels, ti_out->pixels, &roi_in, &roi_out);
  for_testimg_pixels_p_yx(ti_out)
  {
      float *p_in = get_pixel(ti_in, x, y);
      // used in dt when rec2020 is configured: 0.279043, 0.675345, 0.045613
      // float lum = p[0] * 0.2126 + p[1] * 0.7152 + p[2] * 0.0722;
      float lum_in = linear_rgb_luminance(p_in);
      float lum = linear_rgb_luminance(p);
      TR_DEBUG("{%e, %e, %e} lum=%e => {%e, %e, %e} lum=%e",
        p_in[0], p_in[1], p_in[2], lum_in, p[0], p[1], p[2], lum);
    // TODO: assert
  }

  will_return(__wrap_dt_ioppr_get_pipe_work_profile_info, NULL);
  params->preserve_color = DT_FILMIC_METHOD_MAX_RGB;
  printf("preserve_color = MAX_RGB\n");
  commit_params(&module, params, &pipe, &piece);
  process(&module, &piece, ti_in->pixels, ti_out->pixels, &roi_in, &roi_out);
  for_testimg_pixels_p_yx(ti_out)
  {
      float *p_in = get_pixel(ti_in, x, y);
      // used in dt when rec2020 is configured: 0.279043, 0.675345, 0.045613
      // float lum = p[0] * 0.2126 + p[1] * 0.7152 + p[2] * 0.0722;
      float lum_in = linear_rgb_luminance(p_in);
      float lum = linear_rgb_luminance(p);
      TR_DEBUG("{%e, %e, %e} lum=%e => {%e, %e, %e} lum=%e",
        p_in[0], p_in[1], p_in[2], lum_in, p[0], p[1], p[2], lum);
    // TODO: assert
  }

  TR_STEP("max-rgb norm with grey overexposure");
  for_testimg_pixels_p_yx(ti_in)
  {
    p[1] = p[2] = p[0];
  }
  will_return(__wrap_dt_ioppr_get_pipe_work_profile_info, NULL);
  params->preserve_color = DT_FILMIC_METHOD_MAX_RGB;
  printf("preserve_color = MAX_RGB\n");
  commit_params(&module, params, &pipe, &piece);
  process(&module, &piece, ti_in->pixels, ti_out->pixels, &roi_in, &roi_out);
  for_testimg_pixels_p_yx(ti_out)
  {
      float *p_in = get_pixel(ti_in, x, y);
      // used in dt when rec2020 is configured: 0.279043, 0.675345, 0.045613
      // float lum = p[0] * 0.2126 + p[1] * 0.7152 + p[2] * 0.0722;
      float lum_in = linear_rgb_luminance(p_in);
      float lum = linear_rgb_luminance(p);
      TR_DEBUG("{%e, %e, %e} lum=%e => {%e, %e, %e} lum=%e",
        p_in[0], p_in[1], p_in[2], lum_in, p[0], p[1], p[2], lum);
    // TODO: assert
  }
  testimg_free(ti_in);
  testimg_free(ti_out);
}



/*
 * MAIN FUNCTION
 */
int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_full_input_space),
    cmocka_unit_test(test_process)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
