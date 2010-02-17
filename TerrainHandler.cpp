// Terrain handler.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/HeightMapNode.h>
#include "TerrainHandler.h"

TerrainHandler::TerrainHandler(HeightMapNode* node)
    : terrain(node){
    
}

void TerrainHandler::Handle(KeyboardEventArg arg){
    if (arg.type == EVENT_PRESS && arg.sym == KEY_u){
        terrain->SetVertex(128,128, terrain->GetVertex(128, 128)[1] + 10);
        terrain->SetVertex(128,129, terrain->GetVertex(128, 129)[1] + 10);
        terrain->SetVertex(129,128, terrain->GetVertex(129, 128)[1] + 10);
        terrain->SetVertex(129,129, terrain->GetVertex(129, 129)[1] + 10);
    }
    if (arg.type == EVENT_PRESS && arg.sym == KEY_i){
        terrain->SetVertex(128,128, terrain->GetVertex(128, 128)[1] - 10);
        terrain->SetVertex(128,129, terrain->GetVertex(128, 129)[1] - 10);
        terrain->SetVertex(129,128, terrain->GetVertex(129, 128)[1] - 10);
        terrain->SetVertex(129,129, terrain->GetVertex(129, 129)[1] - 10);
    }
    if (arg.type == EVENT_PRESS && arg.sym == KEY_r){
        float* hat = new float[64];
        for (int i = 0; i < 64; ++i)
            hat[i] = 70;
        terrain->SetVertices(57, 57, 8, 8, hat);
        //terrain->SetVertices(-1, -1, 3, 3, hat);
    }
}
