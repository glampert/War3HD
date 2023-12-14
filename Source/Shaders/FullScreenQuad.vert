
// Minimal Vertex Shader that draws a full screen quadrilateral
// in NDC coordinates [-1,+1] to present the Framebuffer
// or to apply further post-processing on a captured frame
void main()
{
    const vec2 scale = vec2(0.5, 0.5);
    vec2 uvs = (gl_Vertex.xy * scale) + scale; // Scale to [0,1] range to make it a valid tex coord.

    gl_TexCoord[0] = vec4(uvs, 0.0, 0.0);
    gl_Position    = vec4(gl_Vertex.xy, 0.0, 1.0);
}
