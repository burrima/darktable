# Artefacts when using RGB-norm in filmicrgb module

Note: filmic means the new filmicrgb module in this article.



## Observations

### Values at input of filmic
Minimum and maximum input values for each color, observed at the input of filmic with all
modules switched off (or with default for those that cannot be disabled) before filmic:

```
min={-1.204780e-01, -1.088885e-01, -1.703739e-01}
max={1.211575e+00, 1.319004e+00, 1.203812e+00}
```

### Unit test with overexposed pixels

Feeding the process() method of the filmic module with different input test images showed a problem
with the max-rgb norm.

When feeding filmic with a grey image that is going from bright grey to an overexposed white, the
output is as expected:

```
[   DEBUG  ] grey_point_source = 9.225000
[   DEBUG  ] black_point_source = -10.550000
[   DEBUG  ] white_point_source = 3.761000
[   DEBUG  ] security_factor = 0.000000
[   DEBUG  ] grey_point_target = 18.450001
[   DEBUG  ] black_point_target = 0.000000
[   DEBUG  ] white_point_target = 100.000000
[   DEBUG  ] output_power = 5.980000
[   DEBUG  ] latitude = 45.000000
[   DEBUG  ] contrast = 1.300000
[   DEBUG  ] saturation = 0.000000
[   DEBUG  ] balance = 12.000000
preserve_color = MAX_RGB
process()
[   DEBUG  ] {4.419416e-01, 4.419416e-01, 4.419416e-01} lum=4.419421e-01 => {7.224076e-01, 7.224076e-01, 7.224076e-01} lum=7.224084e-01
[   DEBUG  ] {4.960629e-01, 4.960629e-01, 4.960629e-01} lum=4.960634e-01 => {7.725912e-01, 7.725912e-01, 7.725912e-01} lum=7.725919e-01
[   DEBUG  ] {5.568116e-01, 5.568116e-01, 5.568116e-01} lum=5.568121e-01 => {8.206729e-01, 8.206729e-01, 8.206729e-01} lum=8.206737e-01
[   DEBUG  ] {6.250000e-01, 6.250000e-01, 6.250000e-01} lum=6.250006e-01 => {8.655158e-01, 8.655158e-01, 8.655158e-01} lum=8.655167e-01
[   DEBUG  ] {7.015386e-01, 7.015386e-01, 7.015386e-01} lum=7.015393e-01 => {9.059040e-01, 9.059040e-01, 9.059040e-01} lum=9.059049e-01
[   DEBUG  ] {7.874507e-01, 7.874507e-01, 7.874507e-01} lum=7.874514e-01 => {9.406146e-01, 9.406146e-01, 9.406146e-01} lum=9.406156e-01
[   DEBUG  ] {8.838832e-01, 8.838832e-01, 8.838832e-01} lum=8.838841e-01 => {9.684460e-01, 9.684460e-01, 9.684460e-01} lum=9.684469e-01
[   DEBUG  ] {9.921256e-01, 9.921256e-01, 9.921256e-01} lum=9.921266e-01 => {9.882730e-01, 9.882730e-01, 9.882730e-01} lum=9.882740e-01
[   DEBUG  ] {1.113623e+00, 1.113623e+00, 1.113623e+00} lum=1.113624e+00 => {9.990746e-01, 9.990746e-01, 9.990746e-01} lum=9.990756e-01
[   DEBUG  ] {1.250000e+00, 1.250000e+00, 1.250000e+00} lum=1.250001e+00 => {1.000001e+00, 1.000001e+00, 1.000001e+00} lum=1.000002e+00
```

Light grey values are lifted at the lower end and going towards display-white for input values
going towards scene-white (correct application of the filmic spline). The overexposed input pixels
are well clipped to the display-white (i.e. 1.0f).

Now, what happens to the same test image when the green and blue channels are darker, so that they
don't clip (i.e. only red is clipping)? This is the result:

```
preserve_color = MAX_RGB
process()
[   DEBUG  ] {4.419416e-01, 3.535533e-01, 3.535533e-01} lum=3.782178e-01 => {6.883527e-01, 5.911070e-01, 5.911070e-01} lum=6.182433e-01
[   DEBUG  ] {4.960629e-01, 3.968503e-01, 3.968503e-01} lum=4.245353e-01 => {7.265440e-01, 6.358953e-01, 6.358953e-01} lum=6.611908e-01
[   DEBUG  ] {5.568116e-01, 4.454493e-01, 4.454493e-01} lum=4.765246e-01 => {7.605137e-01, 6.798226e-01, 6.798226e-01} lum=7.023396e-01
[   DEBUG  ] {6.250000e-01, 5.000000e-01, 5.000000e-01} lum=5.348809e-01 => {7.896041e-01, 7.217938e-01, 7.217938e-01} lum=7.407165e-01
[   DEBUG  ] {7.015386e-01, 5.612309e-01, 5.612309e-01} lum=6.003833e-01 => {8.134254e-01, 7.605165e-01, 7.605165e-01} lum=7.752811e-01
[   DEBUG  ] {7.874507e-01, 6.299605e-01, 6.299605e-01} lum=6.739077e-01 => {8.318920e-01, 7.945722e-01, 7.945722e-01} lum=8.049868e-01
[   DEBUG  ] {8.838832e-01, 7.071066e-01, 7.071066e-01} lum=7.564356e-01 => {8.451481e-01, 8.224787e-01, 8.224787e-01} lum=8.288052e-01
[   DEBUG  ] {9.921256e-01, 7.937005e-01, 7.937005e-01} lum=8.490704e-01 => {8.534533e-01, 8.427997e-01, 8.427997e-01} lum=8.457734e-01
[   DEBUG  ] {1.113623e+00, 8.908985e-01, 8.908985e-01} lum=9.530491e-01 => {8.570073e-01, 8.542461e-01, 8.542461e-01} lum=8.550174e-01
[   DEBUG  ] {1.250000e+00, 1.000000e+00, 1.000000e+00} lum=1.069762e+00 => {8.558095e-01, 8.558095e-01, 8.558095e-01} lum=8.558103e-01
```

It is clearly visible that the values desaturate at the white end (r=g=b) and that the output
luminance is much lower than the input luminance as soon as the red channel is clipping.

For completeness, here are the values when feeding the same image to process() but with different
color preservation modes:

```
preserve_color = NONE
process()
[   DEBUG  ] {4.419416e-01, 3.535533e-01, 3.535533e-01} lum=3.782178e-01 => {7.064054e-01, 6.295821e-01, 6.295821e-01} lum=6.510197e-01
[   DEBUG  ] {4.960629e-01, 3.968503e-01, 3.968503e-01} lum=4.245353e-01 => {7.521010e-01, 6.829011e-01, 6.829011e-01} lum=7.022116e-01
[   DEBUG  ] {5.568116e-01, 4.454493e-01, 4.454493e-01} lum=4.765246e-01 => {7.956771e-01, 7.362174e-01, 7.362174e-01} lum=7.528099e-01
[   DEBUG  ] {6.250000e-01, 5.000000e-01, 5.000000e-01} lum=5.348809e-01 => {8.365778e-01, 7.883533e-01, 7.883533e-01} lum=8.018108e-01
[   DEBUG  ] {7.015386e-01, 5.612309e-01, 5.612309e-01} lum=6.003833e-01 => {8.743164e-01, 8.379345e-01, 8.379345e-01} lum=8.480875e-01
[   DEBUG  ] {7.874507e-01, 6.299605e-01, 6.299605e-01} lum=6.739077e-01 => {9.084591e-01, 8.834636e-01, 8.834636e-01} lum=8.904393e-01
[   DEBUG  ] {8.838832e-01, 7.071066e-01, 7.071066e-01} lum=7.564356e-01 => {9.385026e-01, 9.233954e-01, 9.233954e-01} lum=9.276118e-01
[   DEBUG  ] {9.921256e-01, 7.937005e-01, 7.937005e-01} lum=8.490704e-01 => {9.638026e-01, 9.562464e-01, 9.562464e-01} lum=9.583558e-01
[   DEBUG  ] {1.113623e+00, 8.908985e-01, 8.908985e-01} lum=9.530491e-01 => {9.834750e-01, 9.807304e-01, 9.807304e-01} lum=9.814972e-01
[   DEBUG  ] {1.250000e+00, 1.000000e+00, 1.000000e+00} lum=1.069762e+00 => {9.963191e-01, 9.958242e-01, 9.958242e-01} lum=9.959633e-01
preserve_color = LUMINANCE
process()
[   DEBUG  ] {4.419416e-01, 3.535533e-01, 3.535533e-01} lum=3.782178e-01 => {7.382606e-01, 6.204190e-01, 6.204190e-01} lum=6.533024e-01
[   DEBUG  ] {4.960629e-01, 3.968503e-01, 3.968503e-01} lum=4.245353e-01 => {7.878376e-01, 6.724622e-01, 6.724622e-01} lum=7.046576e-01
[   DEBUG  ] {5.568116e-01, 4.454493e-01, 4.454493e-01} lum=4.765246e-01 => {8.339431e-01, 7.248848e-01, 7.248848e-01} lum=7.553175e-01
[   DEBUG  ] {6.250000e-01, 5.000000e-01, 5.000000e-01} lum=5.348809e-01 => {8.754576e-01, 7.766992e-01, 7.766992e-01} lum=8.042578e-01
[   DEBUG  ] {7.015386e-01, 5.612309e-01, 5.612309e-01} lum=6.003833e-01 => {9.114969e-01, 8.266935e-01, 8.266935e-01} lum=8.503581e-01
[   DEBUG  ] {7.874507e-01, 6.299605e-01, 6.299605e-01} lum=6.739077e-01 => {9.414983e-01, 8.734394e-01, 8.734394e-01} lum=8.924316e-01
[   DEBUG  ] {8.838832e-01, 7.071066e-01, 7.071066e-01} lum=7.564356e-01 => {9.652493e-01, 9.153286e-01, 9.153286e-01} lum=9.292596e-01
[   DEBUG  ] {9.921256e-01, 7.937005e-01, 7.937005e-01} lum=8.490704e-01 => {9.828312e-01, 9.506378e-01, 9.506378e-01} lum=9.596221e-01
[   DEBUG  ] {1.113623e+00, 8.908985e-01, 8.908985e-01} lum=9.530491e-01 => {9.945126e-01, 9.776469e-01, 9.776469e-01} lum=9.823542e-01
[   DEBUG  ] {1.250000e+00, 1.000000e+00, 1.000000e+00} lum=1.069762e+00 => {1.000572e+00, 9.947733e-01, 9.947733e-01} lum=9.963925e-01
```


## Conclusions

The following conclusions have been taken out of above observations:

- Negative values and values >1.0 can happen on the input of filmic, even with all modules
  switched off (or set to default).
- The max-rgb norm darkens white pixels if at least one channel is overexposed (>1.0). In my eyes
  this is unexpected.
