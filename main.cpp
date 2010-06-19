// main
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// OpenEngine stuff
#include <Meta/Config.h>
#include <Logging/Logger.h>
#include <Logging/StreamLogger.h>
#include <Core/Engine.h>
#include <Core/EngineEvents.h>
#include <Display/Camera.h>
#include <Display/Frustum.h>
#include <Display/PerspectiveViewingVolume.h>
#include <Geometry/Mesh.h>
#include <Resources/ResourceManager.h>
#include <Resources/Directory.h>
#include <Resources/Texture3D.h>
#include <Resources/Texture3DFileListResource.h>
#include <Scene/BlendingNode.h>
#include <Scene/MeshNode.h>
#include <Scene/SceneNode.h>
#include <Devices/IMouse.h>
#include <Devices/IKeyboard.h>
#include <Utils/Timer.h>

// SDL extension
#include <Display/SDLEnvironment.h>

// Generic handlers
#include <Utils/MoveHandler.h>
#include <Utils/QuitHandler.h>

// OpenGL stuff
#include <Renderers/OpenGL/Renderer.h>
#include <Resources/OpenGLShader.h>

// Terrain stuff
#include <Renderers/OpenGL/TerrainRenderingView.h>
#include <Display/OpenGL/RenderCanvas.h>
#include "Scene/Island.h"
#include <Scene/GrassNode.h>
#include <Scene/SunNode.h>
#include <Scene/SkySphereNode.h>
#include <Scene/WaterNode.h>
//#include <Resources/SDLImage.h>
#include <Resources/FreeImage.h>
#include <Utils/TerrainUtils.h>
#include <Utils/TerrainTexUtils.h>
//#include <Resources/TGAResource.h>
#include <Utils/PerlinNoise.h>

// Fps stuff
#include <Display/HUD.h>
#include <Utils/FPSSurface.h>
#include <Renderers/TextureLoader.h>
#include <Renderers/OpenGL/ShaderLoader.h>

#include <Display/AntTweakBar.h>
#include <Utils/BetterMoveHandler.h>
#include <Utils/IInspector.h>
#include <Utils/InspectionBar.h>

#include "TerrainHandler.h"

// Mesh stuff
#include <Utils/MeshCreator.h>
#include <Scene/MeshNode.h>

// PostProcess
#include <Scene/PostProcessNode.h>
#include <Scene/ChainPostProcessNode.h>
#include <Scene/UnderwaterPostProcessNode.h>

// name spaces that we will be using.
using namespace OpenEngine;
using namespace OpenEngine::Core;
using namespace OpenEngine::Devices;
using namespace OpenEngine::Display;
using namespace OpenEngine::Geometry;
using namespace OpenEngine::Logging;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Scene;
using namespace OpenEngine::Utils;
using namespace OpenEngine::Utils::MeshCreator;

Engine* engine;
IEnvironment* env;
IFrame* frame;
Display::IRenderCanvas* canvas;
Renderer* renderer;
IMouse* mouse;
IKeyboard* keyboard;
ISceneNode* scene;
Camera* camera;
Frustum* frustum;
IRenderingView* renderingview;
TextureLoader* textureloader;
HUD* hud;

bool useShader = true;

#include <Utils/TextureTool.h>

static float multiplier = 1.0;
static bool showTexCoords = false;
static Vector<3,float> center(0,0,0);
//static Vector<3,float> center(0,-2000,0);

class CloudAnimator
    : public IListener<Core::ProcessEventArg> {
    IShaderResourcePtr shader;
    Time cycleTime;
    Time dt;
    RandomGenerator r;
    float lastI;
    float windAngle, distAngle;
    Vector<3,float> currentPosition;
    Vector<3,float> oldHeadding;
    Vector<3,float> newHeadding;

public:
    CloudAnimator(IShaderResourcePtr shader, unsigned int cycleTime)
        : shader(shader), cycleTime(Time(cycleTime,0)) {
        lastI = 0.0;
        windAngle = 0.0;
        currentPosition = Vector<3,float>(0,0,0);

        distAngle = PI/3;
        Vector<3,float> w(1,0,0);
        windAngle = r.Normal(windAngle, distAngle);
        Quaternion<float> q1(0.0, windAngle, 0.0);
        oldHeadding = q1.RotateVector(w);

        windAngle = r.Normal(windAngle, distAngle);
        Quaternion<float> q2(0.0, windAngle, 0.0);
        newHeadding = q2.RotateVector(w);
    }

    void Handle(Core::ProcessEventArg arg) {
        dt += Time(arg.approx);
        while (dt >= cycleTime) {
            dt -= cycleTime;
        }
        float i = ((float)dt.AsInt())/cycleTime.AsInt();
        if (lastI > i) {
            oldHeadding = newHeadding;
            windAngle = r.Normal(windAngle, distAngle);        
            Vector<3,float> w(1,0,0);
            Quaternion<float> q(0.0, windAngle, 0.0);
            newHeadding = q.RotateVector(w);
            lastI = i = 0.0;            
        }

        Vector<3,float> headding = (1-i) * oldHeadding + i * newHeadding;
        currentPosition += (i-lastI) * headding;

        // clamp
        currentPosition[0] -= floor(currentPosition[0]);
        currentPosition[2] -= floor(currentPosition[2]);

        shader->SetUniform("wind", currentPosition);
        shader->SetUniform("multiplier", multiplier);
        shader->SetUniform("showTexCoords", showTexCoords);
        lastI = i;
    }
};

class GradientAnimator
    : public IListener<Core::ProcessEventArg> {
    IShaderResourcePtr shader;
    Time cycleTime;
    Time dt;
    RandomGenerator r;
    float lastI;
    float windAngle, distAngle;
    Vector<3,float> currentPosition;
    Vector<3,float> oldHeadding;
    Vector<3,float> newHeadding;
    SunNode& sun;

public:
    GradientAnimator(IShaderResourcePtr shader, unsigned int cycleTime, SunNode& sun)
        : shader(shader), cycleTime(Time(cycleTime,0)), sun(sun) {
    }

    void Handle(Core::ProcessEventArg arg) {
        /*
        dt += Time(arg.approx);
        while (dt >= cycleTime) {
            dt -= cycleTime;
        }
        shader->SetUniform("interpolator", GetI());
        */
        //shader->SetUniform("sunDirection", sun.GetPos().GetNormalize());
        shader->SetUniform("timeOfDayRatio", sun.GetTimeofDayRatio());
        //logger.info << "time of day ratio:" << sun.GetTimeofDayRatio() << logger.end;
        //logger.info << "sun position:" << sun.GetPos() << logger.end;
        //logger.info << "sun position length:" << sun.GetPos().GetLength() << logger.end;
    }
    /*
    void SetI(float i) {
        dt = Time(cycleTime.AsInt()*i);
    }
    float GetI() {
        float i = ((float)dt.AsInt())/cycleTime.AsInt();
        return i;
    }
    */
};

namespace OpenEngine {
namespace Utils {
namespace Inspection {
ValueList Inspect(SunNode *sun) {
    ValueList values;

    RWValueCall<SunNode, float > *v
        = new RWValueCall<SunNode, float >
        (*sun,
         &SunNode::GetTimeOfDay,
         &SunNode::SetTimeOfDay);
    v->name = "time of day";
    v->properties[MIN] = 0.0;
    v->properties[MAX] = 24.0;
    v->properties[STEP] = 1/64.0;
    values.push_back(v);
    return values;    
}
}}}

class Delayed3dTextureLoader 
    : public IListener<Renderers::RenderingEventArg> {
private:
    ITexture3DPtr tex;
public:
    Delayed3dTextureLoader(ITexture3DPtr tex) : tex(tex) {}
    void Handle(RenderingEventArg arg) {
        arg.renderer.LoadTexture(tex);
    }
};

class TextureLoadOnInit
    : public IListener<RenderingEventArg> {
    TextureLoader& tl;
public:
    TextureLoadOnInit(TextureLoader& tl) : tl(tl) { }
    void Handle(RenderingEventArg arg) {
        if (arg.canvas.GetScene() != NULL) {
            tl.Load(*arg.canvas.GetScene());
        }
    }
};

class CloudDomeMover
    : public IListener<Core::ProcessEventArg> {
private:
    IViewingVolume& vv;
    TransformationNode& tNode;
public:
    CloudDomeMover(IViewingVolume& vv, TransformationNode& tNode) 
        : vv(vv), tNode(tNode) {}
    void Handle(Core::ProcessEventArg arg) {
        Vector<3,float> position = vv.GetPosition();
        //position[1] = 0; // clamp height
        tNode.SetPosition(position);
    }
};

class RenderStateHandler : public IListener<KeyboardEventArg> {
    RenderStateNode* node;
public:
    RenderStateHandler(RenderStateNode* n) : node(n) {            
        node->DisableOption(RenderStateNode::WIREFRAME);
        node->DisableOption(RenderStateNode::TANGENT);
    }
    void Handle(KeyboardEventArg arg) {
        if (arg.type == EVENT_PRESS && arg.sym == KEY_g){
            node->ToggleOption(RenderStateNode::WIREFRAME);
        }
        if (arg.type == EVENT_PRESS && arg.sym == KEY_b){
            node->ToggleOption(RenderStateNode::TANGENT);
        }

        if (arg.type == EVENT_PRESS && arg.sym == KEY_n){
            multiplier += 0.025;
            if (multiplier > 1.0) multiplier = 1.0;
        }
        if (arg.type == EVENT_PRESS && arg.sym == KEY_m){
            multiplier -= 0.025;
            if (multiplier < 0.0) multiplier = 0.0;
        }
        if (arg.type == EVENT_PRESS && arg.sym == KEY_t){
            showTexCoords = !showTexCoords;
        }
    }
};

// Forward declarations ... ffs c++
void SetupDisplay();
void SetupRendering();

int main(int argc, char** argv) {
    // create a logger to std out    
    Logger::AddLogger(new StreamLogger(&std::cout));
    logger.info << "execution binary: " << argv[0] << logger.end;
    logger.info << "current working directory: " 
                << Directory::GetCWD()<< logger.end;


    // setup the engine
    engine = new Engine;

    SetupDisplay();

    // add plug-ins
    ResourceManager<ITexture2D>::AddPlugin(new FreeImagePlugin());
    ResourceManager<UCharTexture2D>::AddPlugin(new UCharFreeImagePlugin());
    ResourceManager<FloatTexture2D>::AddPlugin(new FloatFreeImagePlugin());
    ResourceManager<IShaderResource>::AddPlugin(new GLShaderPlugin());
    //Texture3DFileListResourcePlugin<float>* irp =
    //    new Texture3DFileListResourcePlugin<float>();
    //ResourceManager< Texture3D<float> >::AddPlugin(irp);

    DirectoryManager::AppendPath("projects/Terrain/data/");

    scene = new SceneNode();

    SetupRendering();

    // Setup fps counter
    FPSSurfacePtr fps = FPSSurface::Create();
    textureloader->Load(fps, TextureLoader::RELOAD_QUEUED);
    engine->ProcessEvent().Attach(*fps);
    hud = new HUD();
    HUD::Surface* fpshud = hud->CreateSurface(fps);
    renderer->PostProcessEvent().Attach(*hud);
    fpshud->SetPosition(HUD::Surface::LEFT, HUD::Surface::TOP);

    // Setup scene
    Vector<2, int> dimension(800, 600);
    /*
    IShaderResourcePtr underwater = ResourceManager<IShaderResource>::Create("extensions/OpenGLPostProcessEffects/shaders/Underwater.glsl");
    UnderwaterPostProcessNode* pp = new UnderwaterPostProcessNode(underwater, dimension);
    */
    std::list<IShaderResourcePtr> effects;
    IShaderResourcePtr glow = ResourceManager<IShaderResource>::Create("shaders/glow.glsl");
    effects.push_back(glow);
    effects.push_back(ResourceManager<IShaderResource>::Create("extensions/OpenGLPostProcessEffects/shaders/HorizontalBoxBlur.glsl"));
    ChainPostProcessNode* glowNode = new ChainPostProcessNode(effects, dimension, 1, true);
    glow->SetTexture("scene", glowNode->GetPostProcessNode(1)->GetSceneFrameBuffer()->GetTexAttachment(0));
    renderer->InitializeEvent().Attach(*glowNode);
    /*
    IShaderResourcePtr motionBlur = ResourceManager<IShaderResource>::Create("extensions/OpenGLPostProcessEffects/shaders/MotionBlur.glsl");
    PostProcessNode* motionblur = new PostProcessNode(motionBlur, dimension);
    */

    std::list<IShaderResourcePtr> dof;
    dof.push_back(ResourceManager<IShaderResource>::Create("extensions/OpenGLPostProcessEffects/shaders/VerticalDepthOfField.glsl"));
    dof.push_back(ResourceManager<IShaderResource>::Create("extensions/OpenGLPostProcessEffects/shaders/HorizontalDepthOfField.glsl"));
    ChainPostProcessNode* depthOfFieldNode = new ChainPostProcessNode(dof, dimension, 1, true);
    renderer->InitializeEvent().Attach(*depthOfFieldNode);

    
    UCharTexture2DPtr tmap = ResourceManager<UCharTexture2D>
        ::Create("textures/heightmap2.tga");
    tmap = ChangeChannels(tmap, 1);
    FloatTexture2DPtr map = ConvertTex(tmap);
    map->SetWrapping(CLAMP_TO_EDGE);
    map->SetColorFormat(LUMINANCE32F);
    BoxBlur(map);
    BoxBlur(map);
    BoxBlur(map);
    const float widthScale = 2.0;
    const float heightScale = 1.5;
    Vector<3, float> origo(map->GetHeight() * widthScale / 2,
                           0,
                           map->GetWidth() * widthScale / 2);

    // setup sun
    Vector<3, float> sunDir = Vector<3, float>(1448, 2048, 1448);
    SunNode* sun = new SunNode(sunDir, origo);
    engine->ProcessEvent().Attach(*sun);

    // Setup terrain
    HeightMapNode* land = new Island(map);
    land->SetHeightScale(heightScale);
    land->SetWidthScale(widthScale);
    land->SetOffset(Vector<3, float>(0, -10.75, 0));
    renderer->InitializeEvent().Attach(*land);
    keyboard->KeyEvent().Attach(*(new TerrainHandler(land)));

    // Setup water
    WaterNode* water = new WaterNode(Vector<3, float>(origo), 2048);
    if (useShader){
        IShaderResourcePtr waterShader = ResourceManager<IShaderResource>
            ::Create("projects/Terrain/data/shaders/water/Water.glsl");
        water->SetWaterShader(waterShader, 64.0);
        UCharTexture2DPtr normalmap = ResourceManager<UCharTexture2D>
            ::Create("textures/waterNormalmap.jpg");
        UCharTexture2DPtr dudvmap = ResourceManager<UCharTexture2D>
            ::Create("textures/waterDistortion.jpg");
        water->SetNormalDudvMap(normalmap, dudvmap);
    }else{
        ITexture2DPtr waterSurface = ResourceManager<ITexture2D>
            ::Create("textures/water.tga");
        water->SetSurfaceTexture(waterSurface, 64.0);
    }
    renderer->InitializeEvent().Attach(*water);
    engine->ProcessEvent().Attach(*water);


    Utils::Timer timer;
    timer.Start();

    // Add Cloud quad.
    IShaderResourcePtr cloudShader = ResourceManager<IShaderResource>::
    Create("projects/Terrain/data/shaders/clouds/Clouds.glsl");
    /*
    FloatTexture2DPtr cloudChannel = 
        PerlinNoise::Generate(512, 512, 128, 0.5, 1, 10, 5, 0);
    PerlinNoise::Smooth(cloudChannel,20);
    PerlinNoise::Normalize(cloudChannel,0,1); 
    PerlinNoise::CloudExpCurve(cloudChannel);
    FloatTexture2DPtr cloudTexture = 
        PerlinNoise::ToRGBAinAlphaChannel(cloudChannel);
    TextureTool::DumpTexture(ToUCharTexture(cloudTexture), "output.png");
    TextureTool::DumpTexture(cloudTexture, "output.exr");
    cloudTexture->SetColorFormat(RGBA32F);
    cloudTexture->SetMipmapping(false);
    cloudTexture->SetCompression(false);
*/

    std::string foldername = "projects/Terrain/data/generated/clouds.3d.exr";
    FloatTexture3DPtr cloudTexture;
    if (Directory::Exists(foldername)) {
        logger.info << "loading 3d texture: " << foldername << logger.end;
        cloudTexture = Texture3DFileListResource<float>::Create(foldername);
    } else {
        logger.info << "generating 3d texture: " << foldername << logger.end;
        FloatTexture3DPtr cloudChannel = 
            PerlinNoise::Generate3D(128, 128, 64,
                                    128, 0.5, 1, 3, 3, 0);
        //PerlinNoise::Smooth3D(cloudChannel,20);
        PerlinNoise::Normalize3D(cloudChannel,0,1); 
        PerlinNoise::CloudExpCurve3D(cloudChannel);
        cloudTexture = 
            PerlinNoise::ToRGBAinAlphaChannel3D(cloudChannel);
        TextureTool<float>::DumpTexture(cloudTexture, foldername);
    }
    cloudTexture->SetMipmapping(false);
    cloudTexture->SetCompression(false);
    cloudShader->SetTexture("clouds", cloudTexture);

    logger.info << "time elapsed: "
                << timer.GetElapsedTime() << logger.end;

    /*
    //from: http://geography.about.com/library/faq/blqzdiameter.htm
    float earth_diameter = 12756.32; //m
    float earth_radius = earth_diameter / 2;
    //from: http://da.wikipedia.org/wiki/Sky_(meteorologi)
    float cloud_radius = earth_radius + 1000;
    */
    MeshPtr clouds = 
        //CreateGeodesicSphere(cloud_radius, 2, false, Vector<3,float>(1.0f));
        CreateGeodesicSphere(1000, 3, false, Vector<3,float>(1.0f));
        //CreateGeodesicSphere(1000, 2, false, Vector<3,float>(1.0f));
        //CreateGeodesicSphere(300, 5, false, Vector<3,float>(1.0f));

    clouds->GetMaterial()->shad = cloudShader;
    MeshNode* cloudNode = new MeshNode();
    cloudNode->SetMesh(clouds);
    RenderStateNode* cloudScene = new RenderStateNode();
    cloudScene->DisableOption(RenderStateNode::DEPTH_TEST);
    ISceneNode* bn = new BlendingNode();
    TransformationNode* cloudPos = new TransformationNode();

    //cloudPos->SetPosition(Vector<3,float>(0,-earth_radius,0));
    //cloudPos->Rotate(PI/2,0,0);
    cloudPos->SetPosition(center);
    cloudPos->AddNode(cloudNode);
    bn->AddNode(cloudPos);
    cloudScene->AddNode(bn);

    CloudDomeMover* cdm = new CloudDomeMover(*camera, *cloudPos);
    engine->ProcessEvent().Attach(*cdm);    

    Delayed3dTextureLoader* d3dtl = new Delayed3dTextureLoader(cloudTexture);
    renderer->InitializeEvent().Attach(*d3dtl);

    CloudAnimator* cAnim = new CloudAnimator(cloudShader, 20);
    engine->ProcessEvent().Attach(*cAnim);

    // gradient dome
    MeshPtr atmosphericDome = 
        CreateGeodesicSphere(3000, 2, false, Vector<3,float>(1.0f));
    IShaderResourcePtr gradientShader = ResourceManager<IShaderResource>::
    Create("projects/Terrain/data/shaders/gradient/Gradient.glsl");
    UCharTexture2DPtr gradient = ResourceManager<UCharTexture2D>
        ::Create("textures/EarthClearSky2.png");
    gradient->SetWrapping(CLAMP_TO_EDGE);
    gradientShader->SetTexture("gradient", gradient);
    atmosphericDome->GetMaterial()->shad = gradientShader;

    // stars
    UCharTexture2DPtr stars =
        UCharTexture2DPtr(new Texture2D<unsigned char>(512,512,1));
    unsigned char* data = stars->GetData();
    RandomGenerator r;
    for (unsigned int n=0; n<200; n++) {
        float dist = r.UniformFloat(0.05, 0.9);
        float angle = r.UniformFloat(0, 2*PI);
        unsigned int x = (unsigned int)(256 + cos(angle) * dist * 256);
        unsigned int y = (unsigned int)(256 + sin(angle) * dist * 256);
        data[x+y*512] = (unsigned char)(256 * r.UniformFloat(0.5, 0.9));
    }
    gradientShader->SetTexture("stars", stars);

    MeshNode* atmosphericNode = new MeshNode();
    atmosphericNode->SetMesh(atmosphericDome);
    RenderStateNode* atmosphericScene = new RenderStateNode();
    //cloudScene->DisableOption(RenderStateNode::DEPTH_TEST);
    //ISceneNode* bn = new BlendingNode();
    TransformationNode* atmosphericDomePosition = new TransformationNode();

    //cloudPos->SetPosition(Vector<3,float>(0,-earth_radius,0));
    //cloudPos->Rotate(PI/2,0,0);
    //cloudPos->SetPosition(center);

    atmosphericDomePosition->AddNode(atmosphericNode);
    atmosphericScene->AddNode(atmosphericDomePosition);
    GradientAnimator* gAnim = new GradientAnimator(gradientShader, 50, *sun);
    engine->ProcessEvent().Attach(*gAnim);

    /*
    // test texture
    MeshPtr testPlane = CreatePlane(100); 
    testPlane->GetMaterial()->AddTexture(stars);
    MeshNode* testNode = new MeshNode();
    testNode->SetMesh(testPlane);
    TransformationNode* tTestNode = new TransformationNode();
    tTestNode->SetPosition(Vector<3,float>(0,3,0));
    tTestNode->AddNode(testNode);
    */

    // Sky sphere node
    /*
    IShaderResourcePtr atmosphere = ResourceManager<IShaderResource>::Create("projects/Terrain/data/shaders/SkyFromAtmosphere/SkyFromAtmosphere.glsl");
    const Vector<3, float> wavelength = Vector<3, float>(0.65f, 0.57f, 0.475f);
    const float outerRadius = 1025.0f;
    const float innerRadius = 1000.0f;
    const float fScale = 1 / (outerRadius - innerRadius);
    const float ESun = 20.0f;
    const float kr = 0.0025f;
    const float km = 0.001f;
    const float g = -0.99f; // The Mie phase asymmetry factor
    atmosphere->SetUniform("v3InvWavelength", Vector<3, float>(1) / (wavelength * wavelength * wavelength * wavelength));
    atmosphere->SetUniform("fCameraHeight", 1.0f);
    atmosphere->SetUniform("fInnerRadius", innerRadius);
    atmosphere->SetUniform("fKrESun", kr * ESun);
    atmosphere->SetUniform("fKmESun", km * ESun);
    atmosphere->SetUniform("fKr4PI", kr * 4 * PI);
    atmosphere->SetUniform("fKm4PI", km * 4 * PI);
    atmosphere->SetUniform("fScale", fScale);
    atmosphere->SetUniform("fScaleDepth", 0.25f);
    atmosphere->SetUniform("fScaleOverScaleDepth", fScale / 0.25f);
    atmosphere->SetUniform("g", g);
    atmosphere->SetUniform("g2", g * g);
    SkySphereNode* sky = new SkySphereNode(atmosphere, outerRadius, 80);
    renderer->InitializeEvent().Attach(*sky);
    */
    // Grass node
    IShaderResourcePtr grassShader = ResourceManager<IShaderResource>::Create("projects/Terrain/data/shaders/grass/Grass.glsl");
    grassShader->SetTexture("heightmap", map);
    GrassNode* grass = new GrassNode(land, grassShader, 12000, 64, 1);
    engine->ProcessEvent().Attach(*grass);
    renderer->InitializeEvent().Attach(*grass);

    // Renderstate node
    RenderStateNode* state = new RenderStateNode();
    state->DisableOption(RenderStateNode::BACKFACE);
    keyboard->KeyEvent().Attach(*(new RenderStateHandler(state)));
    
    // Scene setup
    scene->AddNode(depthOfFieldNode);
    depthOfFieldNode->AddNode(glowNode);
    glowNode->AddNode(water);
    water->AddNode(state);
    state->AddNode(atmosphericScene);
    state->AddNode(cloudScene);
    state->AddNode(grass);
    grass->AddNode(land);
    scene->AddNode(sun);

    //scene->AddNode(tTestNode);

    //state->AddNode(sky);
    /*
    scene->AddNode(state);
    state->AddNode(new MeshNode(MeshPtr(CreateSphere(innerRadius, 15, Vector<3, float>(0,0,0.7)))));
    state->AddNode(sky);
    */

    // ant tweak bar
    AntTweakBar *atb = new AntTweakBar();
    atb->AttachTo(*renderer);
    atb->AddBar(new InspectionBar("time of day",Inspect(sun)));
    keyboard->KeyEvent().Attach(*atb);
    mouse->MouseMovedEvent().Attach(*atb);
    mouse->MouseButtonEvent().Attach(*atb);
    

    // handlers
    BetterMoveHandler *move = new BetterMoveHandler(*camera,
                                                    *mouse,
                                                    true);
    // move->nodes.push_back(lightTrans);

    engine->InitializeEvent().Attach(*move);
    engine->ProcessEvent().Attach(*move);
    atb->KeyEvent().Attach(*move);   
    atb->MouseButtonEvent().Attach(*move);
    atb->MouseMovedEvent().Attach(*move);

    /*     
    // Register the handler as a listener on up and down keyboard events.
    MoveHandler* move_h = new MoveHandler(*camera, *(env->GetMouse()));
    keyboard->KeyEvent().Attach(*move_h);
    engine->InitializeEvent().Attach(*move_h);
    engine->ProcessEvent().Attach(*move_h);
    engine->DeinitializeEvent().Attach(*move_h);
    */
    QuitHandler* quit_h = new QuitHandler(*engine);
    keyboard->KeyEvent().Attach(*quit_h);

    engine->Start();

    // Return when the engine stops.
    return EXIT_SUCCESS;
}

void SetupDisplay(){
    // setup display and devices
    //env = new SDLEnvironment(1440,900,32,FRAME_FULLSCREEN);
    env = new SDLEnvironment(800,600);
    frame    = &env->CreateFrame();
    mouse    = env->GetMouse();
    keyboard = env->GetKeyboard();
    engine->InitializeEvent().Attach(*env);
    engine->ProcessEvent().Attach(*env);
    engine->DeinitializeEvent().Attach(*env);

    // setup camera
    camera  = new Camera(*(new PerspectiveViewingVolume(1, 4000)));
    frustum = new Frustum(*camera);

    camera->SetPosition(Vector<3, float>(-256.0, 200.0, -256.0));
    camera->LookAt(0.0, 127.0, 0.0);
}


void SetupRendering(){
    renderer = new Renderer();
    textureloader = new TextureLoader(*renderer);
    renderingview = new TerrainRenderingView();

    renderer->InitializeEvent().Attach(*renderingview);
    renderer->ProcessEvent().Attach(*renderingview);
    canvas = new Display::OpenGL::RenderCanvas();
    canvas->SetViewingVolume(frustum);
    canvas->SetRenderer(renderer);
    canvas->SetScene(scene);
    frame->SetCanvas(canvas);

    renderer->InitializeEvent()
        .Attach(*(new TextureLoadOnInit(*textureloader)));
 
    renderer->PreProcessEvent().Attach(*textureloader); // needed by fps

    renderer->SetBackgroundColor(Vector<4, float>(0.5, 0.5, 1.0, 1.0));
}
