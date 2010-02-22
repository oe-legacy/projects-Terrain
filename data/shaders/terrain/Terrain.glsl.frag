// material pecular, the alpha channel is the shininess
const vec4 SNOW_SPECULAR = vec4(0.7, 0.7, 1.0, 32.0);
const vec4 GRASS_SPECULAR = vec4(0.0, 0.0, 0.0, 1.0);
const vec4 SAND_SPECULAR = vec4(0.2, 0.2, 0.1, 128.0);
const vec4 CLIFF_SPECULAR = vec4(0.0, 0.0, 0.0, 1.0);

const vec4 WATER_COLOR = vec4(0.0, 0.5, 0.0, 1.0);

const float cliffStartSlope = 0.5;
const float cliffBlend = 0.3;

const float cliffScaling = 4.0;

uniform sampler2D sandTex;
uniform sampler2D sandBump;
uniform sampler2D grassTex;
uniform sampler2D snowTex;
uniform sampler2D cliffTex;
uniform sampler2D cliffBump;
uniform sampler2D normalMap;

uniform vec3 lightDir; // Should be pre-normalized. Or else the world will BURN IN RIGHTEOUS FIRE!!

varying float snowFactor;
varying float grassFactor;
varying float sandFactor;
varying vec3 eyeDir;

void main()
{
    // Extract normal and calculate tangent and binormal
    vec3 normal = texture2D(normalMap, gl_TexCoord[1].xy).xyz;
    vec3 tangent = normalize(vec3(normal.y * -0.5, normal.x, 0.0));
    vec3 binormal = normalize(vec3(0.0, normal.z, normal.y * -0.5));

    // Calculating the texture blend factor
    float snowFactor = clamp(snowFactor, 0.0, 1.0);
    float grassFactor = clamp(grassFactor, 0.0, 1.0);
    float sandFactor = clamp(sandFactor, 0.0, 1.0);

    // calculate cliff factor
    float cliffFactor = (normal.y - cliffStartSlope) / cliffBlend;
    cliffFactor = clamp(cliffFactor, 0.0, 1.0);

    // Calculate bump
    vec3 sandBump = texture2D(sandBump, gl_TexCoord[0].xy).xzy * 2.0 - 1.0;
    sandBump.y *= 3.0;
    vec3 snowBump = sandBump;
    snowBump.y *= 2.0;
    vec3 grassBump = vec3(0.0, 1.0, 0.0);
    vec3 cliffBump = texture2D(cliffBump, gl_TexCoord[0].xy * cliffScaling).xzy * 2.0 - 1.0;
    vec3 bump = mix(grassBump, snowBump, snowFactor);
    bump = mix(sandBump, bump, grassFactor);
    bump = mix(cliffBump, bump, cliffFactor);
    bump = vec3(dot(bump, tangent), dot(bump, normal), dot(bump, binormal));
    normal = normalize(bump);

    // Calculate diffuse
    float ndotl = dot(normal, lightDir);
    float diffuse = clamp(ndotl, 0.0, 1.0);

    // mix shininess and specular
    vec4 matSpecular = mix(GRASS_SPECULAR, SNOW_SPECULAR, snowFactor);
    matSpecular = mix(SAND_SPECULAR, matSpecular, grassFactor);
    matSpecular = mix(CLIFF_SPECULAR, matSpecular, cliffFactor);

    // Calculate specular
    vec3 vRef = normalize(reflect(-lightDir, normal));
    float stemp = clamp(dot(normalize(eyeDir), vRef), 0.0, 1.0);

    vec4 specular = matSpecular * pow(stemp, matSpecular.w);
    
    // Looking up the texture values
    vec2 srcUV = gl_TexCoord[0].xy;
    vec4 grass = texture2D(grassTex, srcUV);
    vec4 snow = texture2D(snowTex, srcUV);
    vec4 sand = texture2D(sandTex, srcUV);
    vec4 cliff = texture2D(cliffTex, srcUV * cliffScaling);

    // Mix textures and add the diffuse
    vec4 text = mix(grass, snow, snowFactor);
    text = mix(sand, text, grassFactor);
    text = mix(cliff, text, cliffFactor);

    vec4 color = text * (gl_LightSource[0].ambient + gl_LightSource[0].diffuse * diffuse) + gl_LightSource[0].specular * specular;

    gl_FragColor = mix(WATER_COLOR, color, sandFactor);
}
