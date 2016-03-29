
varying vec3 v_Normal;

void main()
{
    gl_FragColor = gl_TexCoord[0]; // vec4
    //gl_FragColor = vec4(v_Normal, 1.0);
    //gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
