// material pecular, the alpha channel is the shininess
const vec2 SNOW_SPECULAR = vec2(0.7, 32.0);
const vec2 GRASS_SPECULAR = vec2(0.0, 1.0);
const vec2 SAND_SPECULAR = vec2(0.2, 128.0);
const vec2 CLIFF_SPECULAR = vec2(0.1, 64.0);

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

varying vec2 texCoord;

void main()
{
    vec2 srcUV = texCoord * 16.0;

    // Extract normal and calculate tangent and binormal
    vec3 normal = texture2D(normalMap, texCoord).xyz;
    vec3 tangent = vec3(1.0 - normal.x * normal.x, normal.x, 0.0);
    vec3 binormal = vec3(0.0, normal.z, 1.0 - normal.z * normal.z);

    // Calculating the texture blend factor
    float snowFactor = clamp(snowFactor, 0.0, 1.0);
    float grassFactor = clamp(grassFactor, 0.0, 1.0);
    float sandFactor = clamp(sandFactor, 0.0, 1.0);

    // calculate cliff factor
    float cliffFactor = (normal.y - cliffStartSlope) / cliffBlend;
    cliffFactor = clamp(cliffFactor, 0.0, 1.0);

    // Looking up the texture values
    vec4 grass = texture2D(grassTex, srcUV);
    vec4 snow = texture2D(snowTex, srcUV);
    vec4 sand = texture2D(sandTex, srcUV);
    vec4 cliff = texture2D(cliffTex, srcUV * cliffScaling);

    // Mix textures to get the final texture color
    vec4 text = mix(grass, snow, snowFactor);
    text = mix(sand, text, grassFactor);
    text = mix(cliff, text, cliffFactor);

    // Calculate bump
    vec3 sandBump = texture2D(sandBump, srcUV).xzy;
    vec3 snowBump = sandBump;
    vec3 grassBump = vec3(0.5, 1.0, 0.5);
    vec3 cliffBump = texture2D(cliffBump, srcUV * cliffScaling).xzy;
    vec3 bump = mix(grassBump, snowBump, snowFactor);
    bump = mix(sandBump, bump, grassFactor);
    bump = mix(cliffBump, bump, cliffFactor);
    bump = bump * 2.0 - 1.0;
    bump = vec3(dot(bump, tangent), dot(bump, normal), dot(bump, binormal));
    normal = normalize(bump);

    // Calculate diffuse
    float ndotl = dot(normal, lightDir);
    float diffuse = clamp(ndotl, 0.0, 1.0);

    // mix shininess and specular
    vec2 matSpecular = mix(GRASS_SPECULAR, SNOW_SPECULAR, snowFactor);
    matSpecular = mix(SAND_SPECULAR, matSpecular, grassFactor);
    matSpecular = mix(CLIFF_SPECULAR, matSpecular, cliffFactor);

    // Calculate specular
    vec3 vRef = normalize(reflect(-lightDir, normal));
    float stemp = clamp(dot(normalize(eyeDir), vRef), 0.0, 1.0);
    vec4 specular = text * matSpecular.x * pow(stemp, matSpecular.y);
    
    vec4 color = text * (gl_LightSource[0].ambient + gl_LightSource[0].diffuse * diffuse) + gl_LightSource[0].specular * specular;

    gl_FragColor = mix(WATER_COLOR, color, sandFactor);
}
