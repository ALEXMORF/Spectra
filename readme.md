# Spectra

A real-time SDF pathtracer. Edit your SDFs and see it pathtraced interactively.

[LIVE DEMO](https://youtu.be/u7ObOkmmWpE)

# Build it

Load cl.exe into environment variable, then invoke code/build.bat to generate the binary.

# Usage

Build Spectra and run it. While the application is running, you can edit code/scene.hlsl and the changes will be hotloaded in real-time.

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
...
...

```

Selectively un-comment the scene you want to see. Here are some screenshots of these stock scenes I've prepared:

sphere.hlsl:
![sphere](https://user-images.githubusercontent.com/16845654/83236798-f5191380-a148-11ea-9e33-20f5d9f79d7c.PNG)

cornellbox.hlsl:
![cornellbox](https://user-images.githubusercontent.com/16845654/83236801-f77b6d80-a148-11ea-9a4f-574868cca5a9.PNG)

tron.hlsl:
![tron](https://user-images.githubusercontent.com/16845654/83236803-f9453100-a148-11ea-86e1-8575461cd551.PNG)

coolbox.hlsl:
![coolbox](https://user-images.githubusercontent.com/16845654/83236807-fa765e00-a148-11ea-9dd8-4126e039e73e.PNG)

interior.hlsl:
![interior](https://user-images.githubusercontent.com/16845654/83236809-fba78b00-a148-11ea-99c7-1cc9691dae29.PNG)
