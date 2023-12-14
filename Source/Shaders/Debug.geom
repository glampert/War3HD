
// Polygon outline rendering with the help of a geometry shader:
// https://stackoverflow.com/a/18068177
// https://strattonbrazil.blogspot.com/2011/09/single-pass-wireframe-rendering_10.html
// https://strattonbrazil.blogspot.com/2011/09/single-pass-wireframe-rendering_11.html
// https://web.archive.org/web/20190220052115/http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/

#extension GL_EXT_gpu_shader4 : enable
#extension GL_EXT_geometry_shader4 : enable

uniform vec2 u_ScreenSize;

in vec3 v_VertNormal[3];
in vec4 v_VertColor[3];
in vec4 v_VertTexCoords[3];
in vec4 v_VertWorldPos[3];

out vec3 v_Normal;
out vec4 v_Color;
out vec4 v_TexCoords;
out vec4 v_WorldPos;
noperspective out vec4 v_Distance;

void main()
{
    vec2 scale = u_ScreenSize * vec2(0.5, 0.5);
    vec2 p0 = scale * gl_PositionIn[0].xy / gl_PositionIn[0].w;
    vec2 p1 = scale * gl_PositionIn[1].xy / gl_PositionIn[1].w;
    vec2 p2 = scale * gl_PositionIn[2].xy / gl_PositionIn[2].w;

    vec2 v0 = p2 - p1;
    vec2 v1 = p2 - p0;
    vec2 v2 = p1 - p0;

    float area = abs(v1.x * v2.y - v1.y * v2.x);

    vec4 distV0 = vec4(area / length(v0), 0.0, 0.0, 1.0);
    vec4 distV1 = vec4(0.0, area / length(v1), 0.0, 1.0);
    vec4 distV2 = vec4(0.0, 0.0, area / length(v2), 1.0);

    v_Normal    = v_VertNormal[0];
    v_Color     = v_VertColor[0];
    v_TexCoords = v_VertTexCoords[0];
    v_WorldPos  = v_VertWorldPos[0];
    v_Distance  = distV0;
    gl_Position = gl_PositionIn[0];
    EmitVertex();

    v_Normal    = v_VertNormal[1];
    v_Color     = v_VertColor[1];
    v_TexCoords = v_VertTexCoords[1];
    v_WorldPos  = v_VertWorldPos[1];
    v_Distance  = distV1;
    gl_Position = gl_PositionIn[1];
    EmitVertex();

    v_Normal    = v_VertNormal[2];
    v_Color     = v_VertColor[2];
    v_TexCoords = v_VertTexCoords[2];
    v_WorldPos  = v_VertWorldPos[2];
    v_Distance  = distV2;
    gl_Position = gl_PositionIn[2];
    EmitVertex();

    EndPrimitive();
}
