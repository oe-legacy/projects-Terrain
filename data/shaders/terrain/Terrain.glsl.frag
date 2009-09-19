uniform sampler2D sandTex;
uniform sampler2D grassTex;
uniform sampler2D snowTex;

const vec4 WATER_COLOR = vec4(0.0, 0.5, 0.0, 1.0);

varying vec3 normal;
varying vec3 lightDir;
varying float snowFactor;
varying float grassFactor;
varying float sandFactor;

void main()
{
    // Looking up the texture values
    vec2 srcUV = gl_TexCoord[0].xy;
    vec4 grass = texture2D(grassTex, srcUV);
    vec4 snow = texture2D(snowTex, srcUV);
    vec4 sand = texture2D(sandTex, srcUV);

    // Calculating the texture blend
    float snowFactor = clamp(snowFactor, 0.0, 1.0);
    float grassFactor = clamp(grassFactor, 0.0, 1.0);
    float sandFactor = clamp(sandFactor, 0.0, 1.0);

    vec4 text = mix(grass, snow, snowFactor);
    text = mix(sand, text, grassFactor);

    //vec3 nNormal = normalize(normal);

    float ndotl = dot(normal, lightDir);
    float diffuse = clamp(ndotl, 0.0, 1.0);

    vec4 color = text * gl_LightSource[0].ambient;
    color += text * gl_LightSource[0].diffuse * diffuse;

    gl_FragColor = mix(WATER_COLOR, color, sandFactor);
    gl_FragColor.a = 1.0 - sandFactor;
}
