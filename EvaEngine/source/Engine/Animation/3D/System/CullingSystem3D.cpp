#include "pch.h"
#include "CullingSystem3D.h"
#include <algorithm>
#include <Engine/Scene/Components/Render/3D/Renderable3DComponent.h>
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Animation/3D/VisibleSet.h>
#include <glm/gtc/matrix_access.hpp>

namespace Engine {

    struct Plane { glm::vec4 p; }; // xyz = normal, w = -d (Ax+By+Cz + D = 0 -> store as vec4(A,B,C,D))

    static inline void NormalizePlane(Plane& pl) {
        float invLen = 1.0f / glm::length(glm::vec3(pl.p));
        pl.p *= invLen;
    }

    static void ExtractFrustum(const glm::mat4& VP, Plane out[6]) {
        // Left, Right, Bottom, Top, Near, Far
        out[0].p = glm::row(VP, 3) + glm::row(VP, 0);
        out[1].p = glm::row(VP, 3) - glm::row(VP, 0);
        out[2].p = glm::row(VP, 3) + glm::row(VP, 1);
        out[3].p = glm::row(VP, 3) - glm::row(VP, 1);
        out[4].p = glm::row(VP, 3) + glm::row(VP, 2);
        out[5].p = glm::row(VP, 3) - glm::row(VP, 2);
        for (int i = 0; i < 6; ++i) NormalizePlane(out[i]);
    }

    static bool AABBvsFrustum(const glm::vec3& minW, const glm::vec3& maxW, const Plane fr[6]) {
        // Use positive vertex test per plane
        for (int i = 0; i < 6; ++i) {
            const glm::vec3 n = glm::vec3(fr[i].p);
            glm::vec3 p = minW;
            if (n.x >= 0) p.x = maxW.x;
            if (n.y >= 0) p.y = maxW.y;
            if (n.z >= 0) p.z = maxW.z;
            if (glm::dot(n, p) + fr[i].p.w < 0.0f) return false;
        }
        return true;
    }



    VisibleSet CullingSystem3D::BuildVisible(Scene* scene, const Camera& cam, const TransformSystem3D& xforms, const glm::mat4& cameraWorld)
    {
        EE_PROFILE_FUNCTION();

       

        VisibleSet vis;

        const glm::mat4 V = glm::inverse(cameraWorld);

        glm::mat4 VP = cam.GetProjection() * V;   
        Plane fr[6];
        ExtractFrustum(VP, fr);

        uint32_t culledCount = 0;

        scene->ForEachConst<RenderBoundsComponent>([&](Entity e, const RenderBoundsComponent& rb)
            {
                const glm::mat4* Wptr = xforms.TryGetWorld(e);
                if (!Wptr) return;
                const glm::mat4 W = *Wptr; // copy by value (avoid dangling/mutating memory)

                // Local AABB -> world center/extents via |R| trick
                const glm::vec3 cL = (rb.minL + rb.maxL) * 0.5f;
                const glm::vec3 eHalf = (rb.maxL - rb.minL) * 0.5f;

                const glm::vec3 wc = glm::vec3(W * glm::vec4(cL, 1.0f));
                const glm::mat3 RS = glm::mat3(W);
                const glm::mat3 ARS = glm::mat3(glm::abs(RS[0]), glm::abs(RS[1]), glm::abs(RS[2]));
                const glm::vec3 we = ARS * eHalf;

                // Optional: also compute min/max for your existing AABB path
                const glm::vec3 minW = wc - we;
                const glm::vec3 maxW = wc + we;

                // Per-plane projected-radius test (robust)
                bool inside = true;
                for (int i = 0; i < 6; ++i)
                {
                    const glm::vec3 n = glm::vec3(fr[i].p);  // plane normal
                    const float d = fr[i].p.w;               // plane D, already normalized

                    const float dist = glm::dot(n, wc) + d;              // signed distance of center
                    const float r = glm::dot(glm::abs(n), we);        // projected radius

                    // if center is farther than radius behind plane, it is outside
                    if (dist < -r)
                    {
                        inside = false;
                        break;
                    }
                }

                if (inside)
                    vis.entities.push_back(e);
                else
                    culledCount++;
            });

        EE_CORE_INFO("outside frsutum {}", culledCount);
        return vis;
    }


} 
