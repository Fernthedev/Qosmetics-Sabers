#pragma once

#include "UnityEngine/MonoBehaviour.hpp"
#include "custom-types/shared/macros.hpp"

DECLARE_CLASS_CODEGEN(Qosmetics::Sabers, TrailTransform, UnityEngine::MonoBehaviour,
                      DECLARE_INSTANCE_FIELD(int, trailId);
                      DECLARE_INSTANCE_FIELD(int, isTop);
                      DECLARE_INSTANCE_METHOD(void, Init);

                      public
                      :

                      bool get_isTop();

)