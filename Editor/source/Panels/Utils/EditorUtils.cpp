#include "EditorUtils.h"
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>
#include <imgui.h>
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>

namespace Engine {


    Entity EditorUtils::FindEntityAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition)
    {
        auto tileView = scene->GetRegistry().view<TransformComponent, TileComponent>();

        for (auto entity : tileView)
        {
            const auto& transform = tileView.get<TransformComponent>(entity);
            const auto& tileComp = tileView.get<TileComponent>(entity);

            for (size_t i = 0; i < tileComp.tiles.size(); i++)
            {
                if (tileComp.tiles[i].Category == eTileCategory::Terrain)
                {
                    // prefer other tiles than terrain
                    continue;
                }

                glm::vec2 worldTilePosition = (glm::vec2)transform.Translation + tileComp.tiles[i].position;
                if (worldTilePosition == worldPosition)
                {
                    return Entity{ entity, scene.get() };
                }
            }
        }
        for (auto entity : tileView)
        {
            const auto& transform = tileView.get<TransformComponent>(entity);
            const auto& tileComp = tileView.get<TileComponent>(entity);

            for (size_t i = 0; i < tileComp.tiles.size(); i++)
            {
                if (tileComp.tiles[i].Category != eTileCategory::Terrain)
                {
                    // now look for terrain
                    continue;
                }

                glm::vec2 worldTilePosition = (glm::vec2)transform.Translation + tileComp.tiles[i].position;
                if (worldTilePosition == worldPosition)
                {
                    return Entity{ entity, scene.get() };
                }
            }
        }


        return Entity{};
    }
    void EditorUtils::DeleteTileAtPosition(Entity entity, const glm::vec2& worldPosition)
    {
        if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<TileComponent>())
            return;

        auto& transform = entity.GetComponent<TransformComponent>();
        auto& tileComp = entity.GetComponent<TileComponent>();

        glm::vec2 localOffset = worldPosition - glm::vec2(transform.Translation);

        auto& tiles = tileComp.tiles;
        for (auto it = tiles.begin(); it != tiles.end(); ++it)
        {
            if (glm::all(glm::epsilonEqual(it->position, localOffset, 0.01f))) // Float comparison safety
            {
                tiles.erase(it);
                EE_CORE_INFO("Tile at position ({}, {}) deleted.", localOffset.x, localOffset.y);
                return;
            }
        }

        EE_CORE_WARN("No tile found at local position ({}, {}) to delete.", localOffset.x, localOffset.y);
    }

    std::optional<size_t> EditorUtils::FindTileIndexAtPosition(const TileComponent& tileComp, const TransformComponent& transform, const glm::vec2& worldPos)
    {
        for (size_t i = 0; i < tileComp.tiles.size(); ++i)
        {
            glm::vec2 tileWorldPos = glm::vec2(transform.Translation.x, transform.Translation.y) + tileComp.tiles[i].position;
            if (tileWorldPos == worldPos)
                return i;
        }
        return std::nullopt;
    }


    const char* EditorUtils::AIStateToString(AIState s)
    {
        switch (s)
        {
        case AIState::Idle:         return "Idle";
        case AIState::Patrol:       return "Patrol";
        case AIState::MoveToLastKnown: return "MoveToTarget";
        case AIState::ChaseLOS:     return "ChaseLOS";
        case AIState::Attack:       return "Attack";
        default:                    return "Unknown";
        }
    }


    const char* EditorUtils::EnemyPieceTypeToString(EnemyPieceType t)
    {
        switch (t)
        {
        case EnemyPieceType::Generic:       return "Generic";
        case EnemyPieceType::Head:          return "Head";
        case EnemyPieceType::Torso:         return "Torso";
        case EnemyPieceType::Hip:           return "Hip";
        case EnemyPieceType::ArmL_Upper:    return "ArmL_Upper";
        case EnemyPieceType::ArmL_Forearm:  return "ArmL_Forearm";
        case EnemyPieceType::ArmL_Palm:     return "ArmL_Palm";
        case EnemyPieceType::ArmR_Upper:    return "ArmR_Upper";
        case EnemyPieceType::ArmR_Forearm:  return "ArmR_Forearm";
        case EnemyPieceType::ArmR_Palm:     return "ArmR_Palm";
        case EnemyPieceType::LegL_Thigh:    return "LegL_Thigh";
        case EnemyPieceType::LegL_Calf:     return "LegL_Calf";
        case EnemyPieceType::LegR_Thigh:    return "LegR_Thigh";
        case EnemyPieceType::LegR_Calf:     return "LegR_Calf";
        case EnemyPieceType::Armor1:        return "Armor1";
        case EnemyPieceType::Armor2:        return "Armor2";
        default:                            return "Unknown";
        }
    }

    bool EditorUtils::EnemyPieceTypeCombo(const char* label, EnemyPieceType& type)
    {
        EnemyPieceType current = type;
        bool changed = false;

        if (ImGui::BeginCombo(label, EnemyPieceTypeToString(current)))
        {
            // Keep this list in the same order as the enum
            const EnemyPieceType all[] = {
                EnemyPieceType::Generic,
                EnemyPieceType::Head, EnemyPieceType::Torso, EnemyPieceType::Hip,
                EnemyPieceType::ArmL_Upper, EnemyPieceType::ArmL_Forearm, EnemyPieceType::ArmL_Palm,
                EnemyPieceType::ArmR_Upper, EnemyPieceType::ArmR_Forearm, EnemyPieceType::ArmR_Palm,
                EnemyPieceType::LegL_Thigh, EnemyPieceType::LegL_Calf,
                EnemyPieceType::LegR_Thigh, EnemyPieceType::LegR_Calf,
                EnemyPieceType::Armor1, EnemyPieceType::Armor2
            };

            for (EnemyPieceType t : all)
            {
                bool isSelected = (t == current);
                if (ImGui::Selectable(EnemyPieceTypeToString(t), isSelected))
                {
                    type = t;
                    changed = true;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    Engine::EnemyPiece* EditorUtils::FindPiece(Engine::EnemyDestructibleComponent& destr, Engine::EnemyPieceType type)
    {
        for (auto& p : destr.pieces)
            if (p.type == type)
                return &p;
        return nullptr;
    }

    void EditorUtils::DetachPiece(Engine::Scene* scene, Engine::Entity enemy, Engine::EnemyPieceType type, const glm::vec3& impulseDir,
        float impulseStrength)
    {
        if (!enemy.HasComponent<Engine::EnemyDestructibleComponent>())
            return;

        Engine::EnemyDestructibleComponent& destr = enemy.GetComponent<Engine::EnemyDestructibleComponent>();

        Engine::EnemyPiece* piece = FindPiece(destr, type);
        if (!piece)
            return;

        if (!piece->visible || !piece->canDetach || piece->detached)
            return;

        // 1) Hide piece on the main enemy
        piece->visible = 0;
        piece->detached = 1;
        piece->hitEnabled = 0;

        // 2) Get required components
        if (!enemy.HasComponent<Engine::TransformComponent>() ||
            !enemy.HasComponent<Engine::MeshRefComponent>() ||
            !enemy.HasComponent<Engine::SkeletonComponent>())
        {
            return;
        }

        Engine::TransformComponent& xform = enemy.GetComponent<Engine::TransformComponent>();
        Engine::MeshRefComponent& meshRef = enemy.GetComponent<Engine::MeshRefComponent>();
        Engine::SkeletonComponent& skel = enemy.GetComponent<Engine::SkeletonComponent>();

        const Engine::MeshAsset& mesh = Engine::AssetManager::GetMeshRegistry().GetMesh(meshRef.meshId);

        if (piece->submeshIndex >= mesh.submeshes.size())
        {
            EE_CORE_WARN("[EnemyDestructible] submeshIndex {} out of range (mesh has {})",
                piece->submeshIndex, mesh.submeshes.size());
            return;
        }


        glm::mat4 enemyWorld = xform.GetTransform();


        const auto& sm = mesh.submeshes[piece->submeshIndex];
        glm::vec3 centerL = 0.5f * (sm.aabbMin + sm.aabbMax);
        glm::vec3 spawnPos = glm::vec3(enemyWorld * glm::vec4(centerL, 1.0f));




        Engine::Entity gib = scene->CreateEntity("DetachedPiece");

        Engine::TransformComponent& gx = gib.AddComponent<Engine::TransformComponent>();
        gx.Translation = spawnPos;
        gx.Rotation = xform.Rotation;
        gx.Scale = xform.Scale;

        Engine::MeshRefComponent& gMeshRef = gib.AddComponent<Engine::MeshRefComponent>();
        gMeshRef.meshId = meshRef.meshId;
        gMeshRef.submeshFirst = piece->submeshIndex;
        gMeshRef.submeshCount = 1;

        Engine::RenderBoundsComponent& bounds = gib.AddComponent<Engine::RenderBoundsComponent>();
        bounds.minL = mesh.submeshes[piece->submeshIndex].aabbMin;
        bounds.maxL = mesh.submeshes[piece->submeshIndex].aabbMax;


        PhysicsComponent& phys = gib.AddComponent<PhysicsComponent>();
        phys.velocity = glm::normalize(impulseDir) * impulseStrength;
        phys.gravity = glm::vec3(0.0f, -9.8f, 0.0f);
        phys.active = true;

        float movementTimer = 0.5f;
        phys.duration = movementTimer;
        phys.timeLeft = movementTimer;
        phys.randomizedSpin = true;

        uint32_t skeletonId = 0;


        // this should be removd
        Engine::SkeletonComponent& newSkeleton = gib.AddComponent<Engine::SkeletonComponent>();
        newSkeleton.skeletonId = skeletonId;
        newSkeleton.boneCount = Engine::AssetManager::GetSkeletonRegistry().Get(skeletonId).parent.size();
        newSkeleton.boneBase = 0xFFFFFFFFu;
    }

    eTileDirection EditorUtils::GetDirectionFromTileName(const std::string& tileName)
    {
        if (tileName.empty()) return eTileDirection::Unknown;

        // Get the last character
        char lastChar = tileName.back();

        switch (lastChar)
        {
        case 'N': return eTileDirection::North;
        case 'S': return eTileDirection::South;
        case 'E': return eTileDirection::East;
        case 'W': return eTileDirection::West;
        default:  return eTileDirection::Unknown;
        }
    }


}

