#pragma once

struct NPCDeathComponent
{
    float timeToDespawn = 10.0f;

    float maxDistanceFromPlayer = 50.0f;
    bool  useDistanceCulling = true;

    bool  cleanedUp = false;
};
