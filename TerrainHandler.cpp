// Terrain handler.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/HeightFieldNode.h>
#include "TerrainHandler.h"

TerrainHandler::TerrainHandler(HeightFieldNode* node)
    : terrain(node){
    
}

void TerrainHandler::Handle(KeyboardEventArg arg){
    if (arg.type == EVENT_PRESS && arg.sym == KEY_u)
        terrain->SetVertex(128,128, terrain->GetVertex(128, 128)[1] + 50);
}
