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
process()
min={4.419416e-01, 4.419416e-01, 4.419416e-01}
max={1.250000e+00, 1.250000e+00, 1.250000e+00}
preserve_color: MAX_RGB:
[   DEBUG  ] {4.419416e-01, 4.419416e-01, 4.419416e-01} lum=4.419416e-01 => {7.328916e-01, 7.328916e-01, 7.328916e-01} lum=7.328916e-01
[   DEBUG  ] {4.960629e-01, 4.960629e-01, 4.960629e-01} lum=4.960629e-01 => {7.832901e-01, 7.832901e-01, 7.832901e-01} lum=7.832901e-01
[   DEBUG  ] {5.568116e-01, 5.568116e-01, 5.568116e-01} lum=5.568116e-01 => {8.314527e-01, 8.314527e-01, 8.314527e-01} lum=8.314527e-01
[   DEBUG  ] {6.250000e-01, 6.250000e-01, 6.250000e-01} lum=6.250000e-01 => {8.762432e-01, 8.762432e-01, 8.762432e-01} lum=8.762432e-01
[   DEBUG  ] {7.015386e-01, 7.015386e-01, 7.015386e-01} lum=7.015386e-01 => {9.164720e-01, 9.164720e-01, 9.164720e-01} lum=9.164720e-01
[   DEBUG  ] {7.874507e-01, 7.874507e-01, 7.874507e-01} lum=7.874507e-01 => {9.509450e-01, 9.509450e-01, 9.509450e-01} lum=9.509450e-01
[   DEBUG  ] {8.838832e-01, 8.838832e-01, 8.838832e-01} lum=8.838832e-01 => {9.784827e-01, 9.784827e-01, 9.784827e-01} lum=9.784827e-01
[   DEBUG  ] {9.921256e-01, 9.921256e-01, 9.921256e-01} lum=9.921256e-01 => {9.979903e-01, 9.979903e-01, 9.979903e-01} lum=9.979902e-01
[   DEBUG  ] {1.113623e+00, 1.113623e+00, 1.113623e+00} lum=1.113623e+00 => {1.000000e+00, 1.000000e+00, 1.000000e+00} lum=1.000000e+00
[   DEBUG  ] {1.250000e+00, 1.250000e+00, 1.250000e+00} lum=1.250000e+00 => {1.000000e+00, 1.000000e+00, 1.000000e+00} lum=1.000000e+00
```

Light grey values are lifted at the lower end and going towards display-white for input values
going towards scene-white (correct application of the filmic spline). The overexposed input pixels
are well clipped to the display-white (i.e. 1.0f).

Now, what happens to the same test image when the green and blue channels are darker, so that they
don't clip (i.e. only red is clipping)? This is the result:

```
process()
min={4.419416e-01, 3.535533e-01, 3.535533e-01}
max={1.250000e+00, 1.000000e+00, 1.000000e+00}
preserve_color: MAX_RGB:
[   DEBUG  ] {4.419416e-01, 3.535533e-01, 3.535533e-01} lum=3.732201e-01 => {6.817538e-01, 6.009480e-01, 6.009480e-01} lum=6.189276e-01
[   DEBUG  ] {4.960629e-01, 3.968503e-01, 3.968503e-01} lum=4.189255e-01 => {7.159458e-01, 6.459048e-01, 6.459048e-01} lum=6.614892e-01
[   DEBUG  ] {5.568116e-01, 4.454493e-01, 4.454493e-01} lum=4.702279e-01 => {7.460970e-01, 6.895893e-01, 6.895893e-01} lum=7.021625e-01
[   DEBUG  ] {6.250000e-01, 5.000000e-01, 5.000000e-01} lum=5.278130e-01 => {7.721606e-01, 7.307811e-01, 7.307811e-01} lum=7.399882e-01
[   DEBUG  ] {7.015386e-01, 5.612309e-01, 5.612309e-01} lum=5.924500e-01 => {7.944124e-01, 7.681088e-01, 7.681088e-01} lum=7.739615e-01
[   DEBUG  ] {7.874507e-01, 6.299605e-01, 6.299605e-01} lum=6.650028e-01 => {8.133355e-01, 8.001372e-01, 8.001372e-01} lum=8.030738e-01
[   DEBUG  ] {8.838832e-01, 7.071066e-01, 7.071066e-01} lum=7.464401e-01 => {8.293999e-01, 8.254508e-01, 8.254508e-01} lum=8.263295e-01
[   DEBUG  ] {9.921256e-01, 7.937005e-01, 7.937005e-01} lum=8.378510e-01 => {8.428505e-01, 8.427903e-01, 8.427903e-01} lum=8.428037e-01
[   DEBUG  ] {1.113623e+00, 8.908985e-01, 8.908985e-01} lum=9.404557e-01 => {8.445008e-01, 8.445008e-01, 8.445008e-01} lum=8.445008e-01
[   DEBUG  ] {1.250000e+00, 1.000000e+00, 1.000000e+00} lum=1.055626e+00 => {8.445009e-01, 8.445009e-01, 8.445009e-01} lum=8.445009e-01
```

It is clearly visible that the values desaturate at the white end (r=g=b) and that the output
luminance is much lower than the input luminance as soon as the red channel is clipping.

For completeness, here are the values when feeding the same image to process() but with different
color preservation modes:

```
preserve_color: NONE:
[   DEBUG  ] {4.419416e-01, 3.535533e-01, 3.535533e-01} lum=3.732201e-01 => {7.090297e-01, 6.398963e-01, 6.398963e-01} lum=6.552788e-01
[   DEBUG  ] {4.960629e-01, 3.968503e-01, 3.968503e-01} lum=4.189255e-01 => {7.532651e-01, 6.937420e-01, 6.937420e-01} lum=7.069862e-01
[   DEBUG  ] {5.568116e-01, 4.454493e-01, 4.454493e-01} lum=4.702279e-01 => {7.955533e-01, 7.473334e-01, 7.473334e-01} lum=7.580625e-01
[   DEBUG  ] {6.250000e-01, 5.000000e-01, 5.000000e-01} lum=5.278130e-01 => {8.356106e-01, 7.994407e-01, 7.994407e-01} lum=8.074887e-01
[   DEBUG  ] {7.015386e-01, 5.612309e-01, 5.612309e-01} lum=5.924500e-01 => {8.732273e-01, 8.487000e-01, 8.487000e-01} lum=8.541574e-01
[   DEBUG  ] {7.874507e-01, 6.299605e-01, 6.299605e-01} lum=6.650028e-01 => {9.081024e-01, 8.936569e-01, 8.936569e-01} lum=8.968711e-01
[   DEBUG  ] {8.838832e-01, 7.071066e-01, 7.071066e-01} lum=7.464401e-01 => {9.397491e-01, 9.328912e-01, 9.328912e-01} lum=9.344171e-01
[   DEBUG  ] {9.921256e-01, 7.937005e-01, 7.937005e-01} lum=8.378510e-01 => {9.673001e-01, 9.651083e-01, 9.651083e-01} lum=9.655960e-01
[   DEBUG  ] {1.113623e+00, 8.908985e-01, 8.908985e-01} lum=9.404557e-01 => {9.855551e-01, 9.853082e-01, 9.853082e-01} lum=9.853632e-01
[   DEBUG  ] {1.250000e+00, 1.000000e+00, 1.000000e+00} lum=1.055626e+00 => {9.992357e-01, 9.992325e-01, 9.992325e-01} lum=9.992332e-01
process()
preserve_color: LUMINANCE:
[   DEBUG  ] {4.419416e-01, 3.535533e-01, 3.535533e-01} lum=3.732201e-01 => {7.405093e-01, 6.334748e-01, 6.334748e-01} lum=6.572905e-01
[   DEBUG  ] {4.960629e-01, 3.968503e-01, 3.968503e-01} lum=4.189255e-01 => {7.870351e-01, 6.867894e-01, 6.867894e-01} lum=7.090945e-01
[   DEBUG  ] {5.568116e-01, 4.454493e-01, 4.454493e-01} lum=4.702279e-01 => {8.296049e-01, 7.402979e-01, 7.402979e-01} lum=7.601691e-01
[   DEBUG  ] {6.250000e-01, 5.000000e-01, 5.000000e-01} lum=5.278130e-01 => {8.675588e-01, 7.928931e-01, 7.928931e-01} lum=8.095065e-01
[   DEBUG  ] {7.015386e-01, 5.612309e-01, 5.612309e-01} lum=5.924500e-01 => {9.006417e-01, 8.432277e-01, 8.432277e-01} lum=8.560026e-01
[   DEBUG  ] {7.874507e-01, 6.299605e-01, 6.299605e-01} lum=6.650028e-01 => {9.290186e-01, 8.897553e-01, 8.897553e-01} lum=8.984916e-01
[   DEBUG  ] {8.838832e-01, 7.071066e-01, 7.071066e-01} lum=7.464401e-01 => {9.531724e-01, 9.307973e-01, 9.307973e-01} lum=9.357758e-01
[   DEBUG  ] {9.921256e-01, 7.937005e-01, 7.937005e-01} lum=8.378510e-01 => {9.737042e-01, 9.646537e-01, 9.646537e-01} lum=9.666674e-01
[   DEBUG  ] {1.113623e+00, 8.908985e-01, 8.908985e-01} lum=9.404557e-01 => {9.910577e-01, 9.897273e-01, 9.897273e-01} lum=9.900233e-01
[   DEBUG  ] {1.250000e+00, 1.000000e+00, 1.000000e+00} lum=1.055626e+00 => {1.000000e+00, 1.000000e+00, 1.000000e+00} lum=1.000000e+00
process()
```


## Conclusions

The following conclusions have been taken out of above observations:

- Negative values and values >1.0 can happen on the input of filmic, even with all modules
  switched off (or set to default).
- The max-rgb norm darkens white pixels if at least one channel is overexposed (>1.0). In my eyes
  this is unexpected.
