

// http://relativity.net.au/gaming/glsl/Built-inVariables.html
// https://www.opengl.org/sdk/docs/tutorials/ClockworkCoders/attributes.php

varying vec3 v_Normal;
//varying vec2 v_TexCoords;

void main()
{
    v_Normal = gl_Normal;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = ftransform();
}
