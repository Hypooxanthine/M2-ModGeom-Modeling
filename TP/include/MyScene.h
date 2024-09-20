#pragma once

#include <Vroom/Scene/Scene.h>
#include <Vroom/Render/Camera/FirstPersonCamera.h>
#include <Vroom/Asset/StaticAsset/MeshAsset.h>

#include <glm/gtc/constants.hpp>

#include <vector>

#include "imgui.h"
#include "Bezier.h"

class MyScene : public vrm::Scene
{
public:
	MyScene();
	~MyScene() = default;
 
protected:
	void onInit() override;

	void onEnd() override;

	void onUpdate(float dt) override;

	void onRender() override;

private:
	void computeBezier();
	void updateControlPoints();

	void profile();

	void onImGui();

private:
	struct BezierParams
	{
		union
		{
			int degrees[2] = { 3, 3 };

			struct
			{		
				int degreeU;
				int degreeV;
			};
		};

		union
		{
			int resolutions[2] = { 50, 50 };

			struct
			{		
				int resolutionU;
				int resolutionV;
			};
		};

		union
		{
			float patchSizes[3] = { 10.f, 2.f, 10.f };

			struct
			{		
				float patchSizeU;
				float patchVerticalSpread;
				float patchSizeV;
			};
		};
	};

private:
    vrm::FirstPersonCamera m_Camera;
    float forwardValue = 0.f, rightValue = 0.f, upValue = 0.f;
	float turnRightValue = 0.f, lookUpValue = 0.f;
	float myCameraSpeed = 10.f, myCameraAngularSpeed = .08f * glm::two_pi<float>() / 360.f;
    bool m_MouseLock = false;

	ImFont* m_Font = nullptr;

	bool m_ControlsEnabled = true;
	bool m_RealTimeComputing = true;
	bool m_ShowControlPoints = true;

	vrm::MeshAsset m_MeshAsset;
	Bezier m_Bezier;
	BezierParams m_BezierParams;
	float m_LastComputeTimeSeconds = 0.f;

	std::vector<vrm::Entity> m_ControlPoints;
};