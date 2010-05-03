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
#include <Devices/IMouse.h>
#include <Devices/IKeyboard.h>

// SDL extension
#include <Display/SDLEnvironment.h>

// OpenGL stuff
#include <Renderers/OpenGL/Renderer.h>
#include <Resources/OpenGLShader.h>

// Terrain stuff
#include <Renderers/OpenGL/TerrainRenderingView.h>
#include "Scene/Island.h"
#include <Scene/GrassNode.h>
#include <Scene/SunNode.h>
#include <Scene/SkySphereNode.h>
#include <Scene/WaterNode.h>
#include <Resources/SDLImage.h>
#include <Utils/TerrainUtils.h>
#include <Utils/TerrainTexUtils.h>
#include <Resources/TGAResource.h>

// Fps stuff
#include <Display/HUD.h>
#include <Utils/FPSSurface.h>
#include <Renderers/TextureLoader.h>

// Edit stuff
#include <Utils/MouseSelection.h>
#include <Utils/SelectionSet.h>
#include <Utils/CameraTool.h>
#include <Utils/ToolChain.h>
#include "TerrainHandler.h"

// Mesh stuff
#include <Utils/MeshCreator.h>
#include <Scene/MeshNode.h>

// name spaces that we will be using.
using namespace OpenEngine::Core;
using namespace OpenEngine::Devices;
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

bool useShader = true;

/*
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
*/

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
    const float widthScale = 1.0;
    const float heightScale = 1.0;
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
    GrassNode* grass = new GrassNode(grassShader);
    
    renderer->InitializeEvent().Attach(*grass);

    // Renderstate node
    RenderStateNode* state = new RenderStateNode();
    state->DisableOption(RenderStateNode::BACKFACE);
    keyboard->KeyEvent().Attach(*(new RenderStateHandler(state)));
    
    // Scene setup
    refl->AddNode(state);
    state->AddNode(land);
    land->AddNode(grass);
    scene->AddNode(sun);
    scene->AddNode(water);
    //state->AddNode(sky);
    /*
    scene->AddNode(state);
    state->AddNode(new MeshNode(MeshPtr(CreateSphere(innerRadius, 15, Vector<3, float>(0,0,0.7)))));
    state->AddNode(sky);
    */

    // Setup Edit Tool
    ToolChain* chain = new ToolChain();
    CameraTool* ct = new CameraTool(false);
    chain->PushBackTool(ct);    
    //TerrainEditTool* editTool = new TerrainEditTool(land, frame);
    //chain->PushBackTool(editTool);

    MouseSelection* ms = new MouseSelection(*frame, *mouse, NULL);
    ms->BindTool(viewport, chain);
    
    keyboard->KeyEvent().Attach(*ms);
    mouse->MouseMovedEvent().Attach(*ms);
    mouse->MouseButtonEvent().Attach(*ms);
    renderer->PostProcessEvent().Attach(*ms);

    engine->Start();

    // Return when the engine stops.
    return EXIT_SUCCESS;
}

void SetupDisplay(){
    // setup display and devices
    //env = new SDLEnvironment(1440,900,32,FRAME_FULLSCREEN);
    env = new SDLEnvironment(800,600);
    frame    = &env->GetFrame();
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
    //camera->SetPosition(Vector<3, float>(-1010.0, 0.0, 0.0));
    camera->LookAt(0.0, 127.0, 0.0);
    //camera->SetPosition(Vector<3, float>(1056.0, 200.0, 1056.0));
    //camera->LookAt(800.0, 127.0, 800.0);
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
    //renderer->InitializeEvent().Attach(*(new TextureLoadOnInit(*textureloader)));
    renderer->PreProcessEvent().Attach(*textureloader);

    renderer->SetBackgroundColor(Vector<4, float>(0.5, 0.5, 1.0, 1.0));



}
