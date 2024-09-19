#include "MyScene.h"

#include <Vroom/Core/Application.h>
#include <Vroom/Core/GameLayer.h>
#include <Vroom/Core/Window.h>

#include <Vroom/Scene/Components/MeshComponent.h>
#include <Vroom/Scene/Components/TransformComponent.h>
#include <Vroom/Scene/Components/PointLightComponent.h>

#include <Vroom/Asset/AssetManager.h>

#include <glm/gtx/string_cast.hpp>

#include "imgui.h"

#include "Bezier.h"

MyScene::MyScene()
    : vrm::Scene(), m_Camera(0.1f, 100.f, glm::radians(90.f), 600.f / 400.f, { 0.5f, 0.f, 20.f }, { 0.f, 0.f, 0.f })
{
    auto& gameLayer = vrm::Application::Get().getGameLayer();

    // Bind triggers to the camera
    // This is a bit ugly. I might create some facilities that do this job in the future.
    // Maybe another event type, which will give a scalar depending on the input (moveForward in [-1, 1] for example, controlled with any input we want).
    gameLayer.getTrigger("MoveForward")
        .bindCallback([this](bool triggered) { forwardValue += triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveBackward")
        .bindCallback([this](bool triggered) { forwardValue -= triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveRight")
        .bindCallback([this](bool triggered) { rightValue += triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveLeft")
        .bindCallback([this](bool triggered) { rightValue -= triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveUp")
        .bindCallback([this](bool triggered) { upValue += triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveDown")
        .bindCallback([this](bool triggered) { upValue -= triggered ? 1.f : -1.f; });
    
    gameLayer.getTrigger("MouseLeft")
        .bindCallback([this](bool triggered) {
            if (!m_ControlsEnabled)
                return;
            m_MouseLock = triggered;
            vrm::Application::Get().getWindow().setCursorVisible(!triggered);
        });
    
    gameLayer.getCustomEvent("MouseMoved")
        .bindCallback([this](const vrm::Event& event) {
            turnRightValue += static_cast<float>(event.mouseDeltaX);
            lookUpValue -= static_cast<float>(event.mouseDeltaY);
        });
}

void MyScene::onInit()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    /* ImGui */

    ImGuiIO& io = ImGui::GetIO();
    m_Font = io.Fonts->AddFontFromFileTTF("Resources/Fonts/Roboto/Roboto-Regular.ttf", 24.0f);
    VRM_ASSERT_MSG(m_Font, "Failed to load font.");

    setCamera(&m_Camera);

    /* Visualization */

    computeBezier();

    auto meshEntity = createEntity("Mesh");
    meshEntity.addComponent<vrm::MeshComponent>(m_MeshAsset.createInstance());
    meshEntity.getComponent<vrm::TransformComponent>().setScale({ 10.f, 10.f, 10.f });

    auto lightEntity = createEntity("Light");
    auto& c =  lightEntity.addComponent<vrm::PointLightComponent>();
    c.color = { 1.f, 1.f, 1.f };
    c.intensity = 100.f;
    c.radius = 100.f;
    lightEntity.getComponent<vrm::TransformComponent>().setPosition({ 0.f, 10.f, 10.f });
}

void MyScene::onEnd()
{
}

void MyScene::onUpdate(float dt)
{
    /* Camera */
    if (m_ControlsEnabled && m_MouseLock)
    {
        m_Camera.move(forwardValue * myCameraSpeed * dt * m_Camera.getForwardVector());
        m_Camera.move(rightValue * myCameraSpeed * dt * m_Camera.getRightVector());
        m_Camera.move(upValue * myCameraSpeed * dt * glm::vec3{0.f, 1.f, 0.f});
        m_Camera.addYaw(turnRightValue * myCameraAngularSpeed);
        m_Camera.addPitch(lookUpValue * myCameraAngularSpeed);
    }

    lookUpValue = 0.f;
    turnRightValue = 0.f;
}

void MyScene::onRender()
{
    ImGui::PushFont(m_Font);

    onImGui();

    ImGui::PopFont();
}

void MyScene::onImGui()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::Begin("Controls");
        ImGui::TextWrapped("While left clicking:");
        ImGui::TextWrapped("WASD to move the camera");
        ImGui::TextWrapped("Space to move up, Left Shift to move down");
        ImGui::Checkbox("Enable controls", &m_ControlsEnabled);
        ImGui::TextWrapped("Camera speed");
        ImGui::SliderFloat("##Camera speed", &myCameraSpeed, 0.f, 100.f);
        ImGui::TextWrapped("Camera angular speed");
        ImGui::SliderFloat("##Camera angular speed", &myCameraAngularSpeed, 0.f, 0.1f);
    ImGui::End();

    ImGui::Begin("Tweaks");
        ImGui::Checkbox("Real-time computing", &m_RealTimeComputing);
        ImGui::TextWrapped("Degrees");
        if (ImGui::SliderInt2("##Degrees", m_BezierParams.degrees, 1, 100, "%d", ImGuiSliderFlags_Logarithmic) && m_RealTimeComputing)
            computeBezier();
        ImGui::TextWrapped("Resolution");
        if (ImGui::SliderInt2("##Resolution", m_BezierParams.resolutions, 1, 10000, "%d", ImGuiSliderFlags_Logarithmic) && m_RealTimeComputing)
            computeBezier();
        if (ImGui::Button("Compute Bezier"))
            computeBezier();
    ImGui::End();

    ImGui::Begin("Stats");
        ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
        ImGui::Text("Vertices: %u", m_MeshAsset.getSubMeshes().back().meshData.getVertexCount());
        ImGui::Text("Triangles: %u", m_MeshAsset.getSubMeshes().back().meshData.getTriangleCount());
    ImGui::End();
}

void MyScene::computeBezier()
{
    VRM_LOG_INFO("Computing Bezier with params: degrees: ({}, {}), resolutions: ({}, {})", m_BezierParams.degreeU, m_BezierParams.degreeV, m_BezierParams.resolutionU, m_BezierParams.resolutionV);

    Bezier b(m_BezierParams.degreeU, m_BezierParams.degreeV, m_BezierParams.resolutionU, m_BezierParams.resolutionV);
    
    for (uint32_t u = 0; u < static_cast<uint32_t>(m_BezierParams.degreeU); u++)
    {
        for (uint32_t v = 0; v < static_cast<uint32_t>(m_BezierParams.degreeV); v++)
        {
            glm::vec3 p;
            p.x = static_cast<float>(u);
            p.y = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) / 2.f;
            p.z = static_cast<float>(v);
            b.setControlPoint(u, v, p);
        }
    }

    m_MeshAsset.clear();
    m_MeshAsset.addSubmesh(b.polygonize());
}