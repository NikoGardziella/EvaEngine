#include "pch.h"
#include "Scene.h"
#include "Engine.h"
#include "Engine/UI/UIContext.h"

namespace Engine {

    void Scene::RenderGameUI(UIContext& ui)
    {
        std::stable_sort(ui.elements.begin(), ui.elements.end(),
            [](const auto& a, const auto& b) { return a->tr.layer < b->tr.layer; });

        for (auto& e : ui.elements)
        {

            e->Draw();
        }
    }

}