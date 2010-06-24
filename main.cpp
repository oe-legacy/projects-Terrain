// main
// -------------------------------------------------------------------
// Copyright (C) 2010 OpenEngine.dk (See AUTHORS) 
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
#include <Scene/WaterNode.h>
#include <Resources/FreeImage.h>
#include <Utils/TerrainUtils.h>
#include <Utils/TerrainTexUtils.h>
#include <Utils/ValueNoise.h>

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
    SunNode& sun;

public:
    CloudAnimator(IShaderResourcePtr shader, unsigned int cycleTime, SunNode& sun)
        : shader(shader), cycleTime(Time(cycleTime,0)), sun(sun) {
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
        shader->SetUniform("timeOfDayRatio", sun.GetTimeofDayRatio());
        lastI = i;
    }

    void SetWindCycleTime(float sec) {
        cycleTime = Time((unsigned int)sec,0);
    }

    float GetWindCycleTime() {
        return cycleTime.sec;
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
    IViewingVolume& view;

public:
    GradientAnimator(IShaderResourcePtr shader, unsigned int cycleTime, 
                     SunNode& sun, IViewingVolume& view)
        : shader(shader), cycleTime(Time(cycleTime,0)), sun(sun), view(view) {
    }

    void Handle(Core::ProcessEventArg arg) {
        shader->SetUniform("timeOfDayRatio", sun.GetTimeofDayRatio());
        shader->SetUniform("lightDir", sun.GetPos().GetNormalize());
        shader->SetUniform("viewPos", view.GetPosition());
    }
};


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

namespace OpenEngine {
namespace Utils {
namespace Inspection {
    ValueList PPInspect(ChainPostProcessNode* glow,
                        ChainPostProcessNode* depthOfField,
                        PostProcessNode* motionBlur) {
        ValueList values;
        {
            RWValueCall<ChainPostProcessNode, bool> *v
                = new RWValueCall<ChainPostProcessNode, bool>
                (*glow,
                 &ChainPostProcessNode::Enabled,
                 &ChainPostProcessNode::SetEnabled);
            v->name = "Glow";
            values.push_back(v);
        }
        {
            RWValueCall<ChainPostProcessNode, bool> *v
                = new RWValueCall<ChainPostProcessNode, bool>
                (*depthOfField,
                 &ChainPostProcessNode::Enabled,
                 &ChainPostProcessNode::SetEnabled);
            v->name = "Depth Of Field";
            values.push_back(v);
        }
        {
            RWValueCall<PostProcessNode, bool> *v
                = new RWValueCall<PostProcessNode, bool>
                (*motionBlur,
                 &PostProcessNode::GetEnabled,
                 &PostProcessNode::SetEnabled);
            v->name = "Motion Blur";
            values.push_back(v);
        }

        return values;
    }
ValueList Inspect(SunNode *sun, CloudAnimator *ca) {
    ValueList values;
    {
        RWValueCall<CloudAnimator, float > *v
            = new RWValueCall<CloudAnimator, float >
            (*ca,
             &CloudAnimator::GetWindCycleTime,
             &CloudAnimator::SetWindCycleTime);
        v->name = "wind cycle time";
        v->properties[MIN] = 1;
        v->properties[MAX] = 120;
        v->properties[STEP] = 1;
        values.push_back(v);
    }
    {
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
    }
    {
        RWValueCall<SunNode, float > *v
            = new RWValueCall<SunNode, float >
            (*sun,
             &SunNode::GetDayLength,
             &SunNode::SetDayLength);
        v->name = "day length";
        v->properties[MIN] = 0.0;
        v->properties[MAX] = 120.0;
        v->properties[STEP] = 1.0;
        values.push_back(v);
    }
    return values;    
}



}}}

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

    /*
    // output 2d clouds for the report
    unsigned int octaveFactor = 2;
    unsigned int psize = 64;
    FloatTexture2DPtr cloudChannel = 
        ValueNoise::Generate(psize, psize, 128,
                              1.0/octaveFactor, octaveFactor, 3, 3, 0);
    ValueNoise::Smooth(cloudChannel,20);
    ValueNoise::Normalize(cloudChannel,0,1); 
    ValueNoise::CloudExpCurve(cloudChannel);
    FloatTexture2DPtr cloudTexture2d = 
        ValueNoise::ToRGBAinAlphaChannel(cloudChannel);
    TextureTool<unsigned char>::
        DumpTexture(ValueNoise::ToUCharTexture(cloudTexture2d), "output.png");
    TextureTool<float>::DumpTexture(cloudTexture2d, "output.exr");
    cloudTexture2d->SetColorFormat(RGBA32F);
    cloudTexture2d->SetMipmapping(false);
    cloudTexture2d->SetCompression(false);
    exit(0);
    */

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
    //Vector<2, int> dimension(1440, 900);
    std::list<IShaderResourcePtr> effects;
    IShaderResourcePtr glow = ResourceManager<IShaderResource>::Create("shaders/glow.glsl");
    effects.push_back(glow);
    effects.push_back(ResourceManager<IShaderResource>::Create("shaders/HorizontalCircleBlur.glsl"));
    ChainPostProcessNode* glowNode = new ChainPostProcessNode(effects, dimension, 1, true);
    glow->SetTexture("scene", glowNode->GetPostProcessNode(1)->GetSceneFrameBuffer()->GetTexAttachment(0));
    renderer->InitializeEvent().Attach(*glowNode);

    IShaderResourcePtr motionBlur = ResourceManager<IShaderResource>::Create("extensions/OpenGLPostProcessEffects/shaders/MotionBlur.glsl");
    PostProcessNode* motionBlurNode = new PostProcessNode(motionBlur, dimension);
    motionBlurNode->SetEnabled(false);
    renderer->InitializeEvent().Attach(*motionBlurNode);

    std::list<IShaderResourcePtr> dof;
    dof.push_back(ResourceManager<IShaderResource>::Create("shaders/VerticalDepthOfField.glsl"));
    dof.push_back(ResourceManager<IShaderResource>::Create("shaders/HorizontalDepthOfField.glsl"));
    ChainPostProcessNode* depthOfFieldNode = new ChainPostProcessNode(dof, dimension, 1, true);
    depthOfFieldNode->SetEnabled(true);
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
    sun->SetRenderGeometry(false);
    sun->SetTimeOfDay(6.0f);
    //sun->SetDayLength(0.0f);
    engine->ProcessEvent().Attach(*sun);

    // Setup terrain
    HeightMapNode* land = new Island(map);
    land->SetHeightScale(heightScale);
    land->SetWidthScale(widthScale);
    land->SetOffset(Vector<3, float>(0, -10.75, 0));
    renderer->InitializeEvent().Attach(*land);
    keyboard->KeyEvent().Attach(*(new TerrainHandler(land)));

    // Setup water
    WaterNode* water = new WaterNode(Vector<3, float>(origo), 2560);
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

    // Add Clouds
    IShaderResourcePtr cloudShader = ResourceManager<IShaderResource>::
    Create("projects/Terrain/data/shaders/clouds/Clouds.glsl");

    std::string foldername = "projects/Terrain/data/generated/clouds.3d.exr";
    FloatTexture3DPtr cloudTexture;
    if (Directory::Exists(foldername)) {
        logger.info << "loading 3d texture: " << foldername << logger.end;
        cloudTexture = Texture3DFileListResource<float>::Create(foldername);
    } else {
        logger.info << "generating 3d texture: " << foldername << logger.end;
        FloatTexture3DPtr cloudChannel = 
            ValueNoise::Generate3D(128, 128, 64,
                                    128, 0.5, 1, 3, 3, 0);
        //ValueNoise::Smooth3D(cloudChannel,20);
        ValueNoise::Normalize3D(cloudChannel,0,1); 
        ValueNoise::CloudExpCurve3D(cloudChannel);
        cloudTexture = 
            ValueNoise::ToRGBAinAlphaChannel3D(cloudChannel);
        TextureTool<float>::DumpTexture(cloudTexture, foldername);
    }
    cloudTexture->SetMipmapping(true);
    cloudTexture->SetWrapping(REPEAT);
    cloudTexture->SetCompression(false);
    cloudShader->SetTexture("clouds", (ITexture3DPtr)cloudTexture);

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
    ISceneNode* cloudScene = new BlendingNode();
    TransformationNode* cloudPos = new TransformationNode();

    //cloudPos->SetPosition(Vector<3,float>(0,-earth_radius,0));
    //cloudPos->Rotate(PI/2,0,0);
    cloudPos->SetPosition(center);
    cloudPos->AddNode(cloudNode);
    cloudScene->AddNode(cloudPos);

    CloudDomeMover* cdm = new CloudDomeMover(*camera, *cloudPos);
    engine->ProcessEvent().Attach(*cdm);    

    Delayed3dTextureLoader* d3dtl = new Delayed3dTextureLoader(cloudTexture);
    renderer->InitializeEvent().Attach(*d3dtl);

    CloudAnimator* cAnim = new CloudAnimator(cloudShader, 20, *sun);
    engine->ProcessEvent().Attach(*cAnim);

    // gradient dome
    MeshPtr atmosphericDome = 
        CreateGeodesicSphere(3000, 2, false, Vector<3,float>(1.0f));
    IShaderResourcePtr gradientShader = ResourceManager<IShaderResource>::
    Create("projects/Terrain/data/shaders/gradient/Gradient.glsl");
    UCharTexture2DPtr gradient = ResourceManager<UCharTexture2D>
        ::Create("textures/EarthClearSky2.png");
    gradient->SetWrapping(CLAMP_TO_EDGE);
    gradientShader->SetTexture("gradient", (ITexture2DPtr)gradient);
    atmosphericDome->GetMaterial()->shad = gradientShader;

    // stars
    std::string starDir = "projects/Terrain/data/generated/stars";
    std::string starFile = starDir + "/stars.png";
    UCharTexture2DPtr stars;
    if (File::Exists(starFile)) {
        logger.info << "loading texture: " << starFile << logger.end;
        stars  = ResourceManager<UCharTexture2D>::Create(starFile);
    } else {
        logger.info << "generating texture: " << starFile << logger.end;
        unsigned int ssize = 512; //sampled for the report at 128.
        stars = UCharTexture2DPtr(new Texture2D<unsigned char>(ssize,ssize,1));
        unsigned char* data = stars->GetData();
        RandomGenerator r;
        for (unsigned int n=0; n<200; n++) {
            float dist = r.UniformFloat(0.05, 0.9);
            float angle = r.UniformFloat(0, 2*PI);
            unsigned int x = (unsigned int)
                (ssize/2 + cos(angle) * dist * ssize/2);
            unsigned int y = (unsigned int)
                (ssize/2 + sin(angle) * dist * ssize/2);
            data[x+y*ssize] = (unsigned char)
                (255 * r.UniformFloat(0.5, 0.9));
        }
        Directory::Make(starDir);
        stars = ValueNoise::ToRGBAfromLuminance(stars);
        TextureTool<unsigned char>::DumpTexture(stars, starFile);
    }
	gradientShader->SetTexture("stars", (ITexture2DPtr)stars);

    MeshNode* atmosphericNode = new MeshNode();
    atmosphericNode->SetMesh(atmosphericDome);
    RenderStateNode* atmosphericScene = new RenderStateNode();
    atmosphericScene->DisableOption(RenderStateNode::DEPTH_TEST);
    TransformationNode* atmosphericDomePosition = new TransformationNode();
    atmosphericDomePosition->AddNode(atmosphericNode);
    atmosphericScene->AddNode(atmosphericDomePosition);
    GradientAnimator* gAnim = new GradientAnimator(gradientShader, 50, *sun, *frustum);
    engine->ProcessEvent().Attach(*gAnim);

    logger.info << "time elapsed: "
                << timer.GetElapsedTime() << logger.end;

    // Grass node
    IShaderResourcePtr grassShader = ResourceManager<IShaderResource>
        ::Create("projects/Terrain/data/shaders/grass/Grass.glsl");
    GrassNode* grass = new GrassNode(land, grassShader, 8000, 64, 1);
    engine->ProcessEvent().Attach(*grass);
    renderer->InitializeEvent().Attach(*grass);

    // Renderstate node
    RenderStateNode* state = new RenderStateNode();
    state->DisableOption(RenderStateNode::BACKFACE);
    keyboard->KeyEvent().Attach(*(new RenderStateHandler(state)));
    
    // Scene setup
    scene->AddNode(depthOfFieldNode);
    depthOfFieldNode->AddNode(glowNode);
    glowNode->AddNode(motionBlurNode);
    motionBlurNode->AddNode(water);
    water->AddNode(state);
    state->AddNode(atmosphericScene);
    atmosphericScene->AddNode(cloudScene);
    state->AddNode(grass);
    grass->AddNode(land);
    scene->AddNode(sun);

    // ant tweak bar
    AntTweakBar *atb = new AntTweakBar();
    atb->AttachTo(*renderer);
    atb->AddBar(new InspectionBar("debug variables",Inspect(sun,cAnim)));
    atb->AddBar(new InspectionBar("Post Process Nodes",PPInspect(glowNode,depthOfFieldNode,motionBlurNode)));
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
