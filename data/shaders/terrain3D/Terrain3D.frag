#extension GL_EXT_texture_array : require

const vec3 startHeight = vec3(-10.76, 5.0, 50.0); // {grass, grass, snow}
const vec3 blending = vec3(10.0, 5.0, 20.0); // {grass, grass, snow}

// material specular, scalar and exponent
const vec2 SNOW_SPECULAR = vec2(0.7, 32.0);
const vec2 GRASS_SPECULAR = vec2(0.0, 1.0);
const vec2 SAND_SPECULAR = vec2(0.2, 128.0);
const vec2 CLIFF_SPECULAR = vec2(0.1, 64.0);

const vec4 WATER_COLOR = vec4(0.0, 0.5, 0.0, 1.0);

const float cliffStartSlope = 0.5;
const float cliffBlend = 0.3;

const float cliffScaling = 4.0;

uniform sampler2DArray groundTex;
uniform sampler2DArray normalTex;
uniform sampler2D normalMap;

uniform vec3 lightDir; // Should be pre-normalized. Or else the world will BURN IN RIGHTEOUS FIRE!!

varying float height;

varying vec3 eyeDir;

varying vec2 texCoord;

vec4 phongLighting(in vec4 text, in vec3 normal, in vec2 specProp){
    // Calculate diffuse
    float ndotl = dot(lightDir, normal);
    float diffuse = clamp(ndotl, 0.0, 1.0);
    
    // Calculate specular
    vec3 vRef = normalize(reflect(-lightDir, normal));
    float stemp = clamp(dot(normalize(eyeDir), vRef), 0.0, 1.0);
    float specular = specProp.x * pow(stemp, specProp.y);

    return text * (gl_LightSource[0].ambient + gl_LightSource[0].diffuse * diffuse + gl_LightSource[0].specular * specular);
}

vec4 blinnLighting(in vec4 text, in vec3 normal, in vec2 specProp){
    // Calculate diffuse
    float ndotl = dot(lightDir, normal);
    float diffuse = clamp(ndotl, 0.0, 1.0);
    
    // Calculate specular
    vec3 vRef = normalize(reflect(-lightDir, normal));
    float stemp = clamp(dot(normalize(eyeDir), vRef), 0.0, 1.0);
    vec4 specular = text * specProp.x * pow(stemp, specProp.y);

    return text * (gl_LightSource[0].ambient + gl_LightSource[0].diffuse * diffuse) + gl_LightSource[0].specular * specular;
}

void main()
{
    vec2 srcUV = texCoord * 16.0;

    // Calculating the texture blend factor
    vec3 factors = (vec3(height, height, height) - startHeight) / blending; // Can be calculated in the vertex shader if need be.
    factors = clamp(factors, 0.0, 1.0);

    float l = factors.y + factors.z;
    float layer = floor(l);
    float blend = l - layer;

    // Extract normal and calculate tangent and binormal
    vec3 normal = normalize(texture2D(normalMap, texCoord).xyz);
    vec3 tangent = vec3(1.0 - normal.x * normal.x, normal.x, 0.0);
    vec3 binormal = vec3(0.0, normal.z, 1.0 - normal.z * normal.z);

    // Calculate the cliff factor
    float cliffFactor = (normal.y - cliffStartSlope) / cliffBlend;
    cliffFactor = clamp(cliffFactor, 0.0, 1.0);

    // Texture color
    vec4 text = texture2DArray(groundTex, vec3(srcUV, layer));
    vec4 blendText = texture2DArray(groundTex, vec3(srcUV, layer + 1.0));
    text = mix(text, blendText, blend);

    text = mix(texture2DArray(groundTex, vec3(srcUV * 4.0, 3.0)), text, cliffFactor);

    // Extract normals and transform them into tangent space
    vec3 bumpNormal = texture2DArray(normalTex, vec3(srcUV, layer)).xzy;
    vec3 blendBump = texture2DArray(normalTex, vec3(srcUV, layer + 1.0)).xzy;
    bumpNormal = mix(bumpNormal, blendBump, blend);

    bumpNormal = mix(texture2DArray(normalTex, vec3(srcUV * 4.0, 3.0)).xzy, bumpNormal, cliffFactor);
    bumpNormal = bumpNormal * 2.0 - 1.0;
    bumpNormal = vec3(dot(bumpNormal, tangent), dot(bumpNormal, normal), dot(bumpNormal, binormal));
    bumpNormal = normalize(bumpNormal);

    // Calculate specular
    vec2 matSpecular = factors.x * SAND_SPECULAR + 
        factors.y * GRASS_SPECULAR + 
        factors.z * SNOW_SPECULAR;
    matSpecular = mix(CLIFF_SPECULAR, matSpecular, cliffFactor);

    vec4 color = phongLighting(text, bumpNormal, matSpecular);
    
    gl_FragColor = mix(WATER_COLOR, color, factors.x);
}
