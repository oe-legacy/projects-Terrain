// Island node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _ISLAND_NODE_H_
#define _ISLAND_NODE_H_

#include <Scene/HeightMapNode.h>
#include <Resources/ResourceManager.h>
#include <Resources/IShaderResource.h>
#include <Resources/Texture2D.h>
#include <Resources/Texture3D.h>
#include <Utils/TextureTool.h>

#include <vector>
using std::vector;

string datadir = "projects/Terrain/data/";

namespace OpenEngine {
    namespace Scene {

        class Island : public HeightMapNode {
        protected:
            UCharTexture3DPtr groundTex;
            UCharTexture3DPtr normalTex;
            
        public:
            Island(FloatTexture2DPtr tex)
                : HeightMapNode(tex){
                this->landscapeShader = ResourceManager<IShaderResource>
                    ::Create(datadir+"shaders/terrain3D/Terrain3D.glsl");


                vector<UCharTexture2DPtr> texList;
                std::string foldername = datadir + 
                    "generated/island/colormap.3d.png";
                if (Directory::Exists(foldername)) {
                    logger.info << "loading island coloring textures: "
                                << foldername << logger.end;
                    groundTex = Texture3DFileListResource<unsigned char>
                        ::Create(foldername);
                } else {
                    logger.info << "constructing island coloring texture: " 
                                << foldername << logger.end;

                    // Create the textures and place them in a 3d texture
                    UCharTexture2DPtr sand = ResourceManager<UCharTexture2D>
                        ::Create("textures/sand.jpg");
                    texList.push_back(sand);
                    UCharTexture2DPtr grass = ResourceManager<UCharTexture2D>
                        ::Create("textures/grass.tga");
                    texList.push_back(grass);
                    UCharTexture2DPtr snow = ResourceManager<UCharTexture2D>
                        ::Create("textures/snow.tga");
                    texList.push_back(snow);
                    UCharTexture2DPtr cliff = ResourceManager<UCharTexture2D>
                        ::Create("textures/rockface.jpg");
                    texList.push_back(cliff);
                    groundTex = UCharTexture3DPtr(new Texture3D<unsigned char>
                                                  (texList));
                    
                    TextureTool<unsigned char>::DumpTexture(groundTex,
                                                            foldername);
                }

                texList.clear();
                foldername = datadir +
                    "generated/island/normalmap.3d.png";
                if (Directory::Exists(foldername)) {
                    logger.info << "loading island normal maps: "
                                << foldername << logger.end;
                    normalTex = Texture3DFileListResource<unsigned char>
                        ::Create(foldername);
                } else {
                    logger.info << "constructing island normal maps: " 
                                << foldername << logger.end;


                UCharTexture2DPtr sandNormal = ResourceManager<UCharTexture2D>
                    ::Create("textures/sandBump.jpg");
                texList.push_back(sandNormal);
                UCharTexture2DPtr grassNormal =
                    UCharTexture2DPtr(new Texture2D<unsigned char>(1,1,3));

                //#define useSDLIMAGE
#ifdef useSDLIMAGE
                grassNormal->GetData()[0] = 127;
                grassNormal->GetData()[1] = 127;
                grassNormal->GetData()[2] = 255;
                grassNormal->GetData()[3] = 0;
                grassNormal->SetColorFormat(RGBA);
#else
                grassNormal->GetData()[0] = 255;
                grassNormal->GetData()[1] = 127;
                grassNormal->GetData()[2] = 127;
                grassNormal->SetColorFormat(BGR);
#endif
                texList.push_back(grassNormal);
                texList.push_back(sandNormal);
                UCharTexture2DPtr cliffNormal = ResourceManager<UCharTexture2D>
                    ::Create("textures/rockfaceBump.jpg");
                texList.push_back(cliffNormal);

                normalTex = UCharTexture3DPtr(new Texture3D<unsigned char>
                                              (texList));

                TextureTool<unsigned char>::DumpTexture(normalTex,
                                                        foldername);
                }
            }

            void Initialize(RenderingEventArg arg) {
                groundTex->SetMipmapping(true);
                groundTex->SetUseCase(ITexture3D::TEXTURE2D_ARRAY);
                arg.renderer.LoadTexture(groundTex.get());
                this->landscapeShader->SetTexture("groundTex", groundTex);

                normalTex->SetMipmapping(true);
                normalTex->SetUseCase(ITexture3D::TEXTURE2D_ARRAY);
                arg.renderer.LoadTexture(normalTex.get());
                this->landscapeShader->SetTexture("normalTex", normalTex);

                glBindTexture(GL_TEXTURE_3D, 0);
                glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, 0);
            }

            void PostRender(Display::Viewport view) {
                
            }
        };

    }
}

#endif
