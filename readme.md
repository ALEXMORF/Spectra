# Spectra

A real-time SDF pathtracer. Edit your SDFs and see the changes happen, live!

# Build it

Load cl.exe into environment variable, then invoke code/build.bat to generate the binary.

# Usage

While the application is running, you can edit scene.hlsl and the changes will be hotloaded in real-time. 

To make this work, you need to define the following three functions in scene.hlsl file:

```
// user-defined functions:

point_query Map(float3 P, float Time)
{
  // define geometry
}
material MapMaterial(int MatId, float3 P, float Time)
{
  // define material
}
float3 Env(float3 Rd, float Time)
{
  // define environment lighting
}

```

To see how these functions are getting called, check out raymarch.hlsl.

# Examples

Also, the source comes with some example scenes I've made in the form of include files:

```

#include "sphere.hlsl"
//#include "cornellbox.hlsl"
//#include "tron.hlsl"

```

Selectively un-comment the scene you want to see. Here are some screenshots of these stock scenes I've prepared:

![base profile screenshot 2018 04 22 - 13 22 33 57](https://user-images.githubusercontent.com/16845654/39100342-4f3381fc-463d-11e8-9d3d-3843d40edb53.png)

(Rendered with shaderian, you can find this shader as sample/fractal_pathtracer.frag)

![image](https://user-images.githubusercontent.com/16845654/39226536-ee49b57e-4807-11e8-8be2-33c0174d0f58.png)

(Rendered with shaderian, you can find this shader as sample/menger_sponge.frag)
