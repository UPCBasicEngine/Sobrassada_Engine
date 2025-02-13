#pragma once

#include "LightComponent.h"

class DirectionalLight : public LightComponent{

	public:
		DirectionalLight();
        ~DirectionalLight();

        void EditorParams(const int index);
        float3 GetDirection() const { return direction; }

	private:
        float3 direction;
        
};