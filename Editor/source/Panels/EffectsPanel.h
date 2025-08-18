#pragma once

namespace Engine {

    struct EffectPushConstants;

    class EffectsPanel
    {
    public:
        EffectsPanel();
        explicit EffectsPanel(EffectPushConstants* state);

        // Point the panel at your push-constant struct (e.g., &s_effectPushConstants)
        void SetState(EffectPushConstants& state);

        // Call this each frame while building your ImGui
        void OnImGuiRender();

    private:
        EffectPushConstants* m_state = nullptr;

        // Internal helpers (implemented in .cpp)
        void DrawFlags();
        void DrawTimingAndStrength();
        void DrawColors();
        void DrawCurveAndFlicker();
        void ApplyRecommendedDefaults();
    };

}

