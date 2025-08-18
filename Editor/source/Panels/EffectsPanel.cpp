#include "pch.h"

#include "EffectsPanel.h"
#include <imgui/imgui.h>
#include "Engine/Renderer/VulkanRenderer2D.h"

namespace Engine {

	

    EffectsPanel::EffectsPanel()
    {
    }

    EffectsPanel::EffectsPanel(EffectPushConstants* state)
        : m_state(state)
    {

    }


    void EffectsPanel::SetState(EffectPushConstants& state)
    {
        m_state = &state;
    }

    void EffectsPanel::OnImGuiRender()
    {
        if (m_state == nullptr)
        {
            ImGui::Begin("Effects");
            ImGui::TextUnformatted("EffectPushConstants* is null. Call SetState(&s_effectPushConstants).");
            ImGui::End();
            return;
        }

        if (ImGui::Begin("Effects"))
        {
            // Basic info
            ImGui::TextUnformatted("Push Constants");
            ImGui::Separator();

            // Read-only helpers that are sometimes useful to see
            ImGui::DragFloat2("Texture Origin", &m_state->textureOrigin.x, 0.1f);
            ImGui::DragFloat("Pixel Size", &m_state->pixelSize, 0.01f, 0.0f, 100.0f);
            {
                int texIdx = static_cast<int>(m_state->textureIndex);
                if (ImGui::SliderInt("Texture Index", &texIdx, 0, 8))
                {
                    m_state->textureIndex = static_cast<uint32_t>(texIdx);
                }
            }

            DrawTimingAndStrength();
            DrawFlags();
            DrawColors();
            DrawCurveAndFlicker();

            ImGui::Separator();
            if (ImGui::Button("Reset (recommended)"))
            {
                ApplyRecommendedDefaults();
            }
        }
        ImGui::End();
    }

    void EffectsPanel::DrawTimingAndStrength()
    {
        if (ImGui::CollapsingHeader("Timing and Strength", ImGuiTreeNodeFlags_DefaultOpen))
        {
            {
                int defT = static_cast<int>(m_state->defaultTimer);
                if (ImGui::SliderInt("Default Timer", &defT, 1, 256))
                {
                    m_state->defaultTimer = static_cast<uint32_t>(defT);
                }
            }
            if (ImGui::SliderFloat("Glow Strength", &m_state->glowStrength, 0.0f, 2.0f))
            {
                // nothing else to do
            }
            {
                int maxT = static_cast<int>(m_state->maxTimer);
                if (ImGui::SliderInt("Max Timer (Clamp)", &maxT, 1, 512))
                {
                    m_state->maxTimer = static_cast<uint32_t>(maxT);
                }
            }
        }
    }

    void EffectsPanel::DrawFlags()
    {
        if (ImGui::CollapsingHeader("Flags", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // bit 0 = glow on terrain
            // bit 1 = alpha-lift over empty
            uint32_t flags = m_state->flags;

            bool glowTerrain = (flags & 0x1u) != 0u;
            bool alphaLift = (flags & 0x2u) != 0u;

            if (ImGui::Checkbox("Glow on Terrain (bit 0)", &glowTerrain))
            {
                if (glowTerrain) flags |= 0x1u; else flags &= ~0x1u;
            }
            if (ImGui::Checkbox("Alpha-lift over Empty (bit 1)", &alphaLift))
            {
                if (alphaLift) flags |= 0x2u; else flags &= ~0x2u;
            }

            m_state->flags = flags;
        }
    }

    void EffectsPanel::DrawColors()
    {
        if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float impact[3] = { m_state->impactTint.x,    m_state->impactTint.y,    m_state->impactTint.z };
            float destroyed[3] = { m_state->destroyedTint.x, m_state->destroyedTint.y, m_state->destroyedTint.z };
            float flash[3] = { m_state->flashTint.x,     m_state->flashTint.y,     m_state->flashTint.z };

            // HDR + float to avoid sRGB clamping perception
            ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;

            if (ImGui::ColorEdit3("Impact Tint", impact, flags))
            {
                m_state->impactTint.x = impact[0];
                m_state->impactTint.y = impact[1];
                m_state->impactTint.z = impact[2];
            }
            if (ImGui::ColorEdit3("Destroyed Tint", destroyed, flags))
            {
                m_state->destroyedTint.x = destroyed[0];
                m_state->destroyedTint.y = destroyed[1];
                m_state->destroyedTint.z = destroyed[2];
            }
            if (ImGui::ColorEdit3("Flash Tint (reserved)", flash, flags))
            {
                m_state->flashTint.x = flash[0];
                m_state->flashTint.y = flash[1];
                m_state->flashTint.z = flash[2];
            }

            ImGui::TextUnformatted("Tip: warm explosion look");
            ImGui::BulletText("Impact:    (1.00, 0.90, 0.35)  yellow");
            ImGui::BulletText("Destroyed: (1.00, 0.58, 0.18)  orange");
            ImGui::BulletText("Flash:     (1.00, 0.98, 0.90)  near white");
        }
    }

    void EffectsPanel::DrawCurveAndFlicker()
    {
        if (ImGui::CollapsingHeader("Curve, Flash, Flicker", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // effectParams0:
            // x = flashStrength
            // y = flickerAmount
            // z = alphaLiftEmpty
            // w = curveBoost
            float flashStrength = m_state->effectParams0.x;
            float flickerAmount = m_state->effectParams0.y;
            float alphaLiftEmpty = m_state->effectParams0.z;
            float curveBoost = m_state->effectParams0.w;

            if (ImGui::SliderFloat("Flash Strength", &flashStrength, 0.0f, 1.0f))
            {
                m_state->effectParams0.x = flashStrength;
            }
            if (ImGui::SliderFloat("Flicker Amount", &flickerAmount, 0.0f, 0.5f))
            {
                m_state->effectParams0.y = flickerAmount;
            }
            if (ImGui::SliderFloat("Alpha Lift over Empty", &alphaLiftEmpty, 0.0f, 1.0f))
            {
                m_state->effectParams0.z = alphaLiftEmpty;
            }
            if (ImGui::SliderFloat("Curve Boost (hot start)", &curveBoost, 0.0f, 1.0f))
            {
                m_state->effectParams0.w = curveBoost;
            }
        }
    }

    void EffectsPanel::ApplyRecommendedDefaults()
    {
        // These match the warm explosion feel we discussed.
        m_state->defaultTimer = 48;
        m_state->glowStrength = 0.85f;
        m_state->maxTimer = 64;
        m_state->flags = (1u << 0) | (1u << 1); // glow on terrain + alpha-lift on empty

        m_state->impactTint = { 1.00f, 0.90f, 0.35f, 0.0f }; // yellow
        m_state->destroyedTint = { 1.00f, 0.58f, 0.18f, 0.0f }; // orange
        m_state->flashTint = { 1.00f, 0.98f, 0.90f, 0.0f }; // near white

        m_state->effectParams0 = { 0.30f, 0.07f, 0.85f, 0.80f };
    }

}

