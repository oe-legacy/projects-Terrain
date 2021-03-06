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
#include <Utils/TexUtils.h>
#include <Math/RandomGenerator.h>

#include <vector>
using std::vector;

string datadir = "projects/Terrain/data/";

namespace OpenEngine {
    namespace Scene {

        class Island : public HeightMapNode {
        protected:
            UCharTexture3DPtr groundTex;
            UCharTexture3DPtr normalTex;
            UCharTexture2DPtr dirtTex;
            UCharTexture2DPtr dirtNormalTex;
            
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
                        ::Create("textures/sand.png");
                    texList.push_back(sand);
                    UCharTexture2DPtr grass = ResourceManager<UCharTexture2D>
                        ::Create("textures/grass.png");
                    texList.push_back(grass);
                    UCharTexture2DPtr snow = ResourceManager<UCharTexture2D>
                        ::Create("textures/snow.png");
                    texList.push_back(snow);
                    UCharTexture2DPtr cliff = ResourceManager<UCharTexture2D>
                        ::Create("textures/rockface.png");
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
                    //::Create("textures/newSandNormals.png");
                    ::Create("textures/sandNormals.png");
                texList.push_back(sandNormal);
                UCharTexture2DPtr grassNormal = ResourceManager<UCharTexture2D>
                    ::Create("textures/grassNormals.png");

                grassNormal->Load();
                for (unsigned int u = 0; u < grassNormal->GetWidth(); ++u)
                    for (unsigned int v = 0; v < grassNormal->GetHeight(); ++v){
                        Vector<3, float> pixel;
                        pixel[0] = grassNormal->GetPixel(u, v)[0];
                        pixel[1] = grassNormal->GetPixel(u, v)[1];
                        pixel[2] = grassNormal->GetPixel(u, v)[2];
                        
                        pixel = (pixel / 256.0f) * 2.0 - 1.0;

                        pixel[0] = 1.0;
                        pixel.Normalize();

                        pixel = ((pixel + 1.0f) * 0.5f) * 256.0f;

                        grassNormal->GetPixel(u, v)[0] = pixel[0];
                        grassNormal->GetPixel(u, v)[1] = pixel[1];
                        grassNormal->GetPixel(u, v)[2] = pixel[2];
                    }
                texList.push_back(grassNormal);

                unsigned int w = 1024;
                unsigned int h = 1024;
                UCharTexture2DPtr snowNormal =
                    UCharTexture2DPtr(new Texture2D<unsigned char>(w,h,3));
                snowNormal->SetColorFormat(BGR);
                unsigned char* data = snowNormal->GetData();
                RandomGenerator r;
                for (unsigned int x=0; x<w; x++) {
                    for (unsigned int y=0; y<h; y++) {
                        Vector<3,float> v(1, r.Normal(0, 0.1),
                                          r.Normal(0, 0.1));
                        v.Normalize();
                        
                        data[(x+y*w)*3 + 0] = (v[0] * 0.5 + 0.5) * 256;
                        data[(x+y*w)*3 + 1] = (v[1] * 0.5 + 0.5) * 256;
                        data[(x+y*w)*3 + 2] = (v[2] * 0.5 + 0.5) * 256;
                    }
                }
                texList.push_back(snowNormal);

                UCharTexture2DPtr cliffNormal = ResourceManager<UCharTexture2D>
                    ::Create("textures/rockfaceNormals.png");
                texList.push_back(cliffNormal);

                normalTex = UCharTexture3DPtr(new Texture3D<unsigned char>
                                              (texList));

                TextureTool<unsigned char>::DumpTexture(normalTex,
                                                        foldername);
                }
                
                dirtTex = ResourceManager<UCharTexture2D>
                    ::Create("textures/dirt.png");

                dirtNormalTex = ResourceManager<UCharTexture2D>
                    ::Create("textures/dirtNormals.png");
            }

            void Initialize(RenderingEventArg arg) {
                groundTex->SetMipmapping(true);
                groundTex->SetUseCase(ITexture3D::TEXTURE2D_ARRAY);
                arg.renderer.LoadTexture(groundTex.get());
                this->landscapeShader->SetTexture("groundTex", (ITexture3DPtr)groundTex);

                normalTex->SetMipmapping(true);
                normalTex->SetUseCase(ITexture3D::TEXTURE2D_ARRAY);
                arg.renderer.LoadTexture(normalTex.get());
                this->landscapeShader->SetTexture("normalTex", (ITexture3DPtr)normalTex);

                glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, groundTex->GetID());
                glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_GENERATE_MIPMAP, GL_FALSE);
                CHECK_FOR_GL_ERROR();

                unsigned int width = groundTex->GetWidth();
                unsigned int height = groundTex->GetHeight();
                for (unsigned int l = 0; l < 2; ++l){                    
                    dirtTex = Utils::TexUtils::Scale(dirtTex, width, height);
                    
                    glTexSubImage3D(GL_TEXTURE_2D_ARRAY_EXT,
                                    l,
                                    0,
                                    0,
                                    1,
                                    width,
                                    height,
                                    1,
                                    GL_BGRA,
                                    GL_UNSIGNED_BYTE,
                                    dirtTex->GetData());

                    width /= 2;
                    height /= 2;
                }
                glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_GENERATE_MIPMAP, GL_TRUE);
                dirtTex->Unload();
                CHECK_FOR_GL_ERROR();

                glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, normalTex->GetID());
                glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_GENERATE_MIPMAP, GL_FALSE);
                CHECK_FOR_GL_ERROR();

                width = groundTex->GetWidth();
                height = groundTex->GetHeight();
                for (unsigned int l = 0; l < 2; ++l){                    
                    dirtNormalTex = Utils::TexUtils::Scale(dirtNormalTex, width, height);
                    
                    glTexSubImage3D(GL_TEXTURE_2D_ARRAY_EXT,
                                    l,
                                    0,
                                    0,
                                    1,
                                    width,
                                    height,
                                    1,
                                    GL_BGR,
                                    GL_UNSIGNED_BYTE,
                                    dirtNormalTex->GetData());

                    width /= 2;
                    height /= 2;
                }
                glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_GENERATE_MIPMAP, GL_TRUE);
                dirtNormalTex->Unload();
                CHECK_FOR_GL_ERROR();
            }

            void PostRender(Display::Viewport view) {
                
            }
        };

    }
}

#endif
