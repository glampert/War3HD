
#extension GL_EXT_gpu_shader4 : enable

// Legacy GLSL refs:
// http://relativity.net.au/gaming/glsl/Built-inVariables.html
// https://www.opengl.org/sdk/docs/tutorials/ClockworkCoders/attributes.php

// varyings:
out vec3 v_VertNormal;
out vec4 v_VertColor;
out vec4 v_VertTexCoords;
out vec4 v_VertWorldPos;

void main()
{
    v_VertNormal    = gl_Normal;
    v_VertColor     = gl_Color;
    v_VertTexCoords = gl_MultiTexCoord0;
    v_VertWorldPos  = gl_Vertex;
    gl_Position     = ftransform(); // AKA gl_ModelViewProjectionMatrix * gl_Vertex
}
