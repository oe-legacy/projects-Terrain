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
#include <Display/Viewport.h>
#include <Display/Camera.h>
#include <Display/Frustum.h>
#include <Display/PerspectiveViewingVolume.h>
#include <Geometry/Mesh.h>
#include <Resources/ResourceManager.h>
#include <Resources/Texture3D.h>
#include <Scene/BlendingNode.h>
#include <Scene/MeshNode.h>
#include <Scene/SceneNode.h>
#include <Devices/IMouse.h>
#include <Devices/IKeyboard.h>
#include <Utils/Timer.h>

// SDL extension
#include <Display/SDLEnvironment.h>

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
#include <Resources/SDLImage.h>
#include <Utils/TerrainUtils.h>
#include <Utils/TerrainTexUtils.h>
#include <Resources/TGAResource.h>
#include <Utils/PerlinNoise.h>

// Fps stuff
#include <Display/HUD.h>
#include <Utils/FPSSurface.h>
#include <Renderers/TextureLoader.h>
#include <Renderers/OpenGL/ShaderLoader.h>

/*
// Edit stuff
#include <Utils/MouseSelection.h>
#include <Utils/SelectionSet.h>
#include <Utils/CameraTool.h>
#include <Utils/ToolChain.h>
*/
#include <Utils/MeshCreator.h>
#include "TerrainHandler.h"

// Mesh stuff
#include <Utils/MeshCreator.h>
#include <Scene/MeshNode.h>

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
Viewport* viewport;
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

class ShaderAnimator
    : public IListener<Core::ProcessEventArg> {
    IShaderResourcePtr shader;
    Time cycleTime;
    Time dt;
public:
    ShaderAnimator(IShaderResourcePtr shader, unsigned int cycleTime)
        : shader(shader), cycleTime(Time(cycleTime)) { }
    void Handle(Core::ProcessEventArg arg) {
        dt += Time(arg.approx);
        while (dt >= cycleTime) {
            dt -= cycleTime;
        }
        float i = ((float)dt.AsInt())/cycleTime.AsInt();
        shader->SetUniform("interpolator", i);
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

class QuitHandler : public IListener<KeyboardEventArg> {
    IEngine& engine;
public:
    QuitHandler(IEngine& engine) : engine(engine) {}
    void Handle(KeyboardEventArg arg) {
        if (arg.sym == KEY_ESCAPE) engine.Stop();
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
    }
};

// Forward declarations ... ffs c++
void SetupDisplay();
void SetupRendering();

int main(int argc, char** argv) {
    // create a logger to std out    
    Logger::AddLogger(new StreamLogger(&std::cout));

    // setup the engine
    engine = new Engine;

    SetupDisplay();

    // add plug-ins
    ResourceManager<ITexture2D>::AddPlugin(new SDLImagePlugin());
    ResourceManager<UCharTexture2D>::AddPlugin(new UCharSDLImagePlugin());
    ResourceManager<IShaderResource>::AddPlugin(new GLShaderPlugin());
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

    // bind default keys
    keyboard->KeyEvent().Attach(*(new QuitHandler(*engine)));

    // Setup scene
    UCharTexture2DPtr tmap = ResourceManager<UCharTexture2D>::Create("textures/heightmap2.tga");
    tmap = ChangeChannels(tmap, 1);
    FloatTexture2DPtr map = ConvertTex(tmap);
    map->SetWrapping(CLAMP_TO_EDGE);
    map->SetColorFormat(LUMINANCE32F);
    BoxBlur(map);
    BoxBlur(map);
    BoxBlur(map);
    const float widthScale = 2.0;
    const float heightScale = 1.5;
    Vector<3, float> origo = Vector<3, float>(map->GetHeight() * widthScale / 2, 0, map->GetWidth() * widthScale / 2);

    // setup sun
    Vector<3, float> sunDir = Vector<3, float>(1448, 2048, 1448);
    SunNode* sun = new SunNode(sunDir, origo);
    engine->ProcessEvent().Attach(*sun);

    // Setup terrain
    HeightMapNode* land = new Island(map);
    /*HeightMapNode* land = new HeightMapNode(map);
    if (useShader){
        IShaderResourcePtr landShader = ResourceManager<IShaderResource>::Create("projects/Terrain/data/shaders/terrain/Terrain.glsl");
        land->SetLandscapeShader(landShader);
    }*/
    land->SetHeightScale(heightScale);
    land->SetWidthScale(widthScale);
    land->SetOffset(Vector<3, float>(0, -10.75, 0));
    land->SetSun(sun);
    renderer->InitializeEvent().Attach(*land);
    keyboard->KeyEvent().Attach(*(new TerrainHandler(land)));

    // Reflection scene for water
    ISceneNode* refl = new SceneNode();

    // Setup water
    WaterNode* water = new WaterNode(Vector<3, float>(origo), 2048);
    if (useShader){
        IShaderResourcePtr waterShader = ResourceManager<IShaderResource>::Create("projects/Terrain/data/shaders/water/Water.glsl");
        water->SetWaterShader(waterShader, 64.0);
        UCharTexture2DPtr normalmap = ResourceManager<UCharTexture2D>::Create("textures/waterNormalMap.jpg");
        UCharTexture2DPtr dudvmap = ResourceManager<UCharTexture2D>::Create("textures/waterDistortion.jpg");
        water->SetNormalDudvMap(normalmap, dudvmap);
    }else{
        ITexture2DPtr waterSurface = ResourceManager<ITexture2D>::Create("textures/water.tga");
        water->SetSurfaceTexture(waterSurface, 64.0);
    }
    water->SetReflectionScene(refl);
    water->SetSun(sun);
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
    CloudExpCurve(cloudChannel);
    FloatTexture2DPtr cloudTexture = 
        ToRGBAinAlphaChannel(cloudChannel);
    TextureTool::DumpTexture(ToUCharTexture(cloudTexture), "output.png");
    TextureTool::DumpTexture(cloudTexture, "output.exr");

    cloudTexture->SetColorFormat(RGBA32F);
    cloudTexture->SetMipmapping(false);
    cloudTexture->SetCompression(false);
    */

    FloatTexture3DPtr cloudChannel = 
        PerlinNoise::Generate3D(128, 128, 64,
                                128, 0.5, 1, 2, 3, 0);
    //PerlinNoise::Smooth3D(cloudChannel,20);
    PerlinNoise::Normalize3D(cloudChannel,0,1); 
    PerlinNoise::CloudExpCurve3D(cloudChannel);
    FloatTexture3DPtr cloudTexture = 
        PerlinNoise::ToRGBAinAlphaChannel3D(cloudChannel);

    TextureTool::DumpTexture(cloudTexture,"output");
    logger.info << "execution time: " << timer.GetElapsedTime() << logger.end;

    cloudTexture->SetColorFormat(RGBA32F);
    cloudTexture->SetMipmapping(false);
    cloudTexture->SetCompression(false);

    cloudShader->SetTexture("clouds", cloudTexture);
    MeshPtr clouds = CreatePlane(200.0);
    clouds->GetMaterial()->shad = cloudShader;
    MeshNode* cloudNode = new MeshNode();
    cloudNode->SetMesh(clouds);
    BlendingNode* cloudScene = new BlendingNode();
    cloudScene->AddNode(cloudNode);

    ShaderAnimator* sAnim = new ShaderAnimator(cloudShader, 1000000);
    engine->ProcessEvent().Attach(*sAnim);


    // Sky sphere node
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

    // Grass node
    IShaderResourcePtr grassShader = ResourceManager<IShaderResource>::Create("projects/Terrain/data/shaders/grass/Grass.glsl");
    grassShader->SetTexture("heightmap", map);
    grassShader->SetUniform("scale", Vector<3, float>(widthScale, heightScale, widthScale));
    GrassNode* grass = new GrassNode(land, grassShader);
    engine->ProcessEvent().Attach(*grass);
    
    renderer->InitializeEvent().Attach(*grass);

    // Renderstate node
    RenderStateNode* state = new RenderStateNode();
    state->DisableOption(RenderStateNode::BACKFACE);
    keyboard->KeyEvent().Attach(*(new RenderStateHandler(state)));
    
    // Scene setup
    refl->AddNode(state);
    state->AddNode(grass);
    grass->AddNode(land);
    scene->AddNode(sun);
    scene->AddNode(water);

    scene->AddNode(cloudScene);

    //state->AddNode(sky);
    /*
    scene->AddNode(state);
    state->AddNode(new MeshNode(MeshPtr(CreateSphere(innerRadius, 15, Vector<3, float>(0,0,0.7)))));
    state->AddNode(sky);
    */

    /*
    // Setup Edit Tool
    ToolChain* chain = new ToolChain();
    CameraTool* ct = new CameraTool(false);
    chain->PushBackTool(ct);    

    MouseSelection* ms = new MouseSelection(*frame, *mouse, NULL);
    ms->BindTool(viewport, chain);
    
    keyboard->KeyEvent().Attach(*ms);
    mouse->MouseMovedEvent().Attach(*ms);
    mouse->MouseButtonEvent().Attach(*ms);
    renderer->PostProcessEvent().Attach(*ms);
    */

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

    // setup a default viewport and camera
    viewport = new Viewport(*frame);
    camera  = new Camera(*(new PerspectiveViewingVolume(1, 4000)));
    frustum = new Frustum(*camera);
    viewport->SetViewingVolume(frustum);

    camera->SetPosition(Vector<3, float>(-256.0, 200.0, -256.0));
    camera->LookAt(0.0, 127.0, 0.0);
}


void SetupRendering(){
    renderer = new Renderer();
    textureloader = new TextureLoader(*renderer);
    renderingview = new TerrainRenderingView(*viewport);

    renderer->ProcessEvent().Attach(*renderingview);
    canvas = new Display::OpenGL::RenderCanvas();
    canvas->SetViewingVolume(camera);
    canvas->SetRenderer(renderer);
    canvas->SetScene(scene);
    frame->SetCanvas(canvas);

    renderer->InitializeEvent()
        .Attach(*(new TextureLoadOnInit(*textureloader)));

    renderer->SetBackgroundColor(Vector<4, float>(0.5, 0.5, 1.0, 1.0));
}
