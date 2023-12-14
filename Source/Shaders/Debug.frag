
#extension GL_EXT_gpu_shader4 : enable

// NOTE: Matching constants in ShaderProgram.hpp
const int kDebugViewNone          = 0;
const int kDebugViewTexCoords     = 1;
const int kDebugViewVertNormals   = 2;
const int kDebugViewVertColors    = 3;
const int kDebugViewVertPositions = 4;
const int kDebugViewPolyOutlines  = 5;

uniform int u_DebugView;

in vec3 v_Normal;
in vec4 v_Color;
in vec4 v_TexCoords;
in vec4 v_WorldPos;
noperspective in vec4 v_Distance;

// If defined, use the vertex color for polygon outline rendering; black-and-white otherwise.
#define COLOR_POLY_OUTLINES

// Display mesh tex coords/normals/vertex color as the fragment color.
void main()
{
    vec4 color = vec4(1.0);

    if (u_DebugView == kDebugViewTexCoords)
    {
        color = v_TexCoords;
    }
    else if (u_DebugView == kDebugViewVertNormals)
    {
        color = vec4(v_Normal, 1.0);
    }
    else if (u_DebugView == kDebugViewVertColors)
    {
        color = v_Color;
    }
    else if (u_DebugView == kDebugViewVertPositions)
    {
        color = v_WorldPos;
    }
    else if (u_DebugView == kDebugViewPolyOutlines)
    {
        // Determine frag distance to closest edge
        float nearD = min(min(v_Distance[0], v_Distance[1]), v_Distance[2]);
        float edgeIntensity = exp2(-1.0 * nearD * nearD);

        // Very simple simulated light gradient
        vec3 L = normalize(vec3(1.0, 1.0, 1.0));
        float NdotL = max(dot(v_Normal, L), 0.0);
        const vec4 ambient = vec4(0.2);

        #ifdef COLOR_POLY_OUTLINES
            vec4 diffuse = clamp(NdotL, 0.0, 1.0) * v_Color;
        #else
            vec4 diffuse = clamp(NdotL, 0.0, 1.0) * vec4(1.0);
        #endif

        // Blend between edge color and normal lighting color
        color = (edgeIntensity * vec4(0.1, 0.1, 0.1, 1.0)) + ((1.0 - edgeIntensity) * (ambient + diffuse));
    }

    gl_FragColor = color;
}
