// Terrain handler.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Devices/IKeyboard.h>

namespace OpenEngine {
    namespace Scene {
        class HeightMapNode;
    }
}

using namespace OpenEngine::Devices;
using namespace OpenEngine::Scene;

class TerrainHandler : public IListener<KeyboardEventArg> {
private:
    HeightMapNode* terrain;
public:
    TerrainHandler(HeightMapNode* node);
    ~TerrainHandler() {}

    void Handle(KeyboardEventArg arg);
};
