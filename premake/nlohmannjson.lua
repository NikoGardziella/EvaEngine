project "nlohmannjson"
    kind "Utility"   -- header-only
    language "C++"
    staticruntime "off"

    local ROOT = _MAIN_SCRIPT_DIR
    local json = path.join(ROOT, "EvaEngine/vendor/nlohmannjson")

    includedirs
    {
        json .. "/include"
    }

    files
    {
        json .. "/include/**.hpp"
    }
