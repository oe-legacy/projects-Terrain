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
#include <Resources/ResourceManager.h>
#include <Scene/SceneNode.h>

// SDL extension
#include <Display/SDLEnvironment.h>

// OpenGL stuff
#include <Renderers/OpenGL/Renderer.h>
#include <Resources/GLSLResource.h>

// Terrain stuff
#include <Renderers/OpenGL/TerrainRenderingView.h>
#include <Scene/LandscapeNode.h>
#include <Scene/SunNode.h>
#include <Scene/TerrainModule.h>
#include <Scene/WaterNode.h>
#include <Resources/SDLImage.h>

// Camera stuff
#include <Utils/MoveHandler.h>

// Fps stuff
#include <Display/HUD.h>
#include <Utils/FPSSurface.h>
#include <Renderers/TextureLoader.h>

// name spaces that we will be using.
using namespace OpenEngine::Core;
using namespace OpenEngine::Display;
using namespace OpenEngine::Logging;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Scene;
using namespace OpenEngine::Utils;

Engine* engine;
IEnvironment* env;
IFrame* frame;
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

class TextureLoadOnInit
    : public IListener<RenderingEventArg> {
    TextureLoader& tl;
public:
    TextureLoadOnInit(TextureLoader& tl) : tl(tl) { }
    void Handle(RenderingEventArg arg) {
        if (arg.renderer.GetSceneRoot() != NULL)
            tl.Load(*arg.renderer.GetSceneRoot());
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
    ResourceManager<ITextureResource>::AddPlugin(new SDLImagePlugin());
    ResourceManager<IShaderResource>::AddPlugin(new GLSLPlugin());
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
    IShaderResourcePtr landShader = ResourceManager<IShaderResource>::Create("projects/Terrain/data/shaders/terrain/Terrain.glsl");
    ITextureResourcePtr tgamapPtr = ResourceManager<ITextureResource>::Create("heightmap.tga");
    LandscapeNode* land = new LandscapeNode(tgamapPtr, landShader, 0.5, 1.0);
    land->CloseBorder(64);
    land->SetTextureDetail(8);
    renderer->InitializeEvent().Attach(*land);
    
    // setup sun
    float sunDir[] = {1024, 1024, 1024};
    float origo[] = {land->GetDepth() / 2, 0, land->GetWidth() / 2};
    SunNode* sun = new SunNode(sunDir, origo);
    land->SetSunPos(sun->GetPos());

    // Setup water
    IShaderResourcePtr waterShader = ResourceManager<IShaderResource>::Create("projects/Terrain/data/shaders/water/Water.glsl");
    ITextureResourcePtr waterSurface = ResourceManager<ITextureResource>::Create("textures/water.tga");
    WaterNode* water = new WaterNode(Vector<3, float>(origo), 1024);
    water->SetSurfaceTexture(waterSurface, 4);
    water->SetReflectionScene(land);
    //water->SetWaterShader(waterShader);
    renderer->InitializeEvent().Attach(*water);
    
    // Scene setup
    scene->AddNode(land);
    scene->AddNode(sun);
    scene->AddNode(water);

    // Terrain Module
    TerrainModule* tm = new TerrainModule(sun, land);
    engine->ProcessEvent().Attach(*tm);

    engine->Start();

    // Return when the engine stops.
    return EXIT_SUCCESS;
}

void SetupDisplay(){
    // setup display and devices
    env = new SDLEnvironment(800,600);
    frame    = &env->GetFrame();
    mouse    = env->GetMouse();
    keyboard = env->GetKeyboard();
    engine->InitializeEvent().Attach(*env);
    engine->ProcessEvent().Attach(*env);
    engine->DeinitializeEvent().Attach(*env);

    // setup a default viewport and camera
    viewport = new Viewport(*frame);
    camera  = new Camera(*(new PerspectiveViewingVolume(50, 1500)));
    frustum = new Frustum(*camera);
    viewport->SetViewingVolume(frustum);

    // movecamera
    MoveHandler* move = new MoveHandler(*camera, *mouse);
    keyboard->KeyEvent().Attach(*move);
    camera->SetPosition(Vector<3, float>(-127.0, 250.0, -127.0));
    camera->LookAt(127.0, 127.0, 127.0);
    engine->InitializeEvent().Attach(*move);
    engine->ProcessEvent().Attach(*move);
}

void SetupRendering(){
    renderer = new Renderer(viewport);
    textureloader = new TextureLoader(*renderer);
    renderingview = new TerrainRenderingView(*viewport);

    engine->InitializeEvent().Attach(*renderer);
    engine->ProcessEvent().Attach(*renderer);
    engine->DeinitializeEvent().Attach(*renderer);

    renderer->ProcessEvent().Attach(*renderingview);
    renderer->SetSceneRoot(scene);
    renderer->InitializeEvent().Attach(*(new TextureLoadOnInit(*textureloader)));
    renderer->PreProcessEvent().Attach(*textureloader);

    renderer->SetBackgroundColor(Vector<4, float>(0.5, 0.5, 1.0, 1.0));
}
