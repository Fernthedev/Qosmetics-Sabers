#include "CustomTypes/SaberModelController.hpp"
#include "ConstStrings.hpp"
#include "CustomTypes/SaberModelContainer.hpp"
#include "CustomTypes/WhackerHandler.hpp"
#include "config.hpp"
#include "logging.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/MeshFilter.hpp"
#include "UnityEngine/MeshRenderer.hpp"

#include "sombrero/shared/FastColor.hpp"
#include "sombrero/shared/FastQuaternion.hpp"

#include "GlobalNamespace/SaberModelController.hpp"
#include "GlobalNamespace/SaberTrail.hpp"
#include "GlobalNamespace/SaberTrailRenderer.hpp"
#include "GlobalNamespace/SaberType.hpp"

#include "Trail/TrailPoint.hpp"

DEFINE_TYPE(Qosmetics::Sabers, SaberModelController);

extern std::string JsonValueToString(const rapidjson::Value& val);

namespace Qosmetics::Sabers
{
    void SaberModelController::Init(GlobalNamespace::Saber* saber)
    {
        this->saber = saber;
        auto saberModelContainer = SaberModelContainer::get_instance();

        if (!saberModelContainer || !saberModelContainer->currentSaberObject)
        {
            EditDefaultSaber();
            return;
        }

        auto& config = saberModelContainer->GetSaberConfig();
        auto& globalConfig = Config::get_config();
        int saberType = saber->get_saberType().value;

        StringW saberName(saberType == 0 ? ConstStrings::LeftSaber() : ConstStrings::RightSaber());
        DEBUG("Spawning {} prefab", saberName);
        auto prefab = saberModelContainer->currentSaberObject->get_transform()->Find(saberName)->get_gameObject();

        auto customSaber = UnityEngine::Object::Instantiate(prefab, get_transform()->get_parent());
        auto customSaberT = customSaber->get_transform();

        customSaberT->set_localPosition(Sombrero::FastVector3::zero());
        customSaberT->set_localScale(Sombrero::FastVector3(globalConfig.saberWidth, globalConfig.saberWidth, 1.0f));
        customSaberT->set_localRotation(Sombrero::FastQuaternion::identity());

        // custom saber object instantiated! clean up default saber now
        HideDefaultSaberElements();

        const Sombrero::FastColor leftColor(colorManager->ColorForSaberType(0));
        const Sombrero::FastColor rightColor(colorManager->ColorForSaberType(1));

        const Sombrero::FastColor& thisColor = saberType == 0 ? leftColor : rightColor;
        const Sombrero::FastColor& thatColor = saberType == 1 ? leftColor : rightColor;

        // Make sure all the components on the custom saber are inited
        auto whackerHandler = customSaber->GetComponentInChildren<WhackerHandler*>();

        DEBUG("SaberType: {}", saberType);
        whackerHandler->colorHandler = whackerHandler->get_gameObject()->GetComponentsInChildren<Qosmetics::Sabers::WhackerColorHandler*>().FirstOrDefault();
        whackerHandler->SetColor(thisColor, thatColor);
        whackerHandler->SetSize(globalConfig.saberWidth, globalConfig.saberLength);

        // Make trails work
        switch (globalConfig.trailType)
        {
        case Config::TrailType::BASEGAME:
        default:
            CreateDefaultTrailCopy(customSaberT, whackerHandler);
        case Config::TrailType::CUSTOM:
            whackerHandler->SetupTrails();
            for (auto trail : whackerHandler->trailHandlers)
            {
                trail->SetColor(leftColor, rightColor);
            }
        case Config::TrailType::NONE:
            break;
        }
    }

    void SaberModelController::HideDefaultSaberElements()
    {
        auto saberModelContainer = SaberModelContainer::get_instance();
        auto& config = saberModelContainer->GetSaberConfig();
        auto& globalConfig = Config::get_config();

        if (config.get_hasTrail())
        {
            DEBUG("Removing default trial");
            auto saberModelController = GetComponent<GlobalNamespace::SaberModelController*>();
            auto trail = saberModelController->dyn__saberTrail();
            trail->set_enabled(false);
            trail->dyn__trailRenderer()->set_enabled(false);
        }

        DEBUG("Removing default mesh objects");
        auto filters = get_gameObject()->GetComponentsInChildren<UnityEngine::MeshFilter*>(true);
        for (auto filter : filters)
        {
            auto filterGO = filter->get_gameObject();
            auto name = filterGO->get_name();
            if (config.get_keepFakeGlow() && (name == "FakeGlow0" || name == "FakeGlow1"))
                continue;
            filterGO->SetActive(false);
        }
    }

    void SaberModelController::EditDefaultSaber()
    {
        auto& globalConfig = Config::get_config();

        // Saber size
        auto t = saber->get_transform();
        if (!t)
            return;
        auto basicSaberModel = t->Find(ConstStrings::BasicSaberModelClone());
        if (!basicSaberModel)
            return;
        basicSaberModel->set_localScale(UnityEngine::Vector3(globalConfig.saberWidth, globalConfig.saberWidth, globalConfig.saberLength));

        switch (globalConfig.trailType)
        {
        case Config::TrailType::CUSTOM:
        case Config::TrailType::BASEGAME:
        default:
        {
            auto whackerHandler = basicSaberModel->get_gameObject()->AddComponent<WhackerHandler*>();
            CreateDefaultTrailCopy(basicSaberModel, whackerHandler);
            whackerHandler->SetupTrails();
            for (auto trail : whackerHandler->trailHandlers)
            {
                trail->SetColor(colorManager->ColorForSaberType(0), colorManager->ColorForSaberType(1));
            }
        }
        case Config::TrailType::NONE:
            DEBUG("Removing default trial");
            auto saberModelController = GetComponent<GlobalNamespace::SaberModelController*>();
            if (!saberModelController)
                break;
            auto trail = saberModelController->dyn__saberTrail();
            if (!trail)
                break;
            trail->set_enabled(false);
            trail->dyn__trailRenderer()->set_enabled(false);
        }
    }

    void SaberModelController::CreateDefaultTrailCopy(UnityEngine::Transform* parent, WhackerHandler* handler)
    {
        auto trailObject = UnityEngine::GameObject::New_ctor(ConstStrings::DefaultTrail());
        auto trailObjectT = trailObject->get_transform();
        trailObjectT->SetParent(parent, false);
        trailObjectT->set_localPosition(Sombrero::FastVector3::zero());
        trailObjectT->set_localScale(Sombrero::FastVector3::one());
        trailObjectT->set_localRotation(Sombrero::FastQuaternion::identity());

        auto trailStart = UnityEngine::GameObject::New_ctor(ConstStrings::TrailStart());
        auto trailStartT = trailStart->get_transform();
        trailStartT->get_transform()->SetParent(trailObjectT, false);
        trailStartT->set_localPosition({0.0f, 0.0f, 0.0f});

        trailObjectT->set_localScale(Sombrero::FastVector3::one());
        trailObjectT->set_localRotation(Sombrero::FastQuaternion::identity());

        auto trailEnd = UnityEngine::GameObject::New_ctor(ConstStrings::TrailEnd());
        auto trailEndT = trailEnd->get_transform();
        trailEndT->get_transform()->SetParent(trailObjectT, false);
        trailEndT->set_localPosition({0.0f, 0.0f, 1.0f});

        TrailData trailData(100, saber->get_saberType().value, Sombrero::FastColor::white(), Sombrero::FastColor::white(), 20, 0.2f);
        TrailPoint topPoint(100, true);
        TrailPoint bottomPoint(100, false);

        rapidjson::Document doc;
        auto& allocator = doc.GetAllocator();

        auto trailText = trailObject->AddComponent<UnityEngine::UI::Text*>();
        trailText->set_text(JsonValueToString(trailData.ToJson(allocator)));
        auto topText = trailEnd->AddComponent<UnityEngine::UI::Text*>();
        topText->set_text(JsonValueToString(topPoint.ToJson(allocator)));
        auto botText = trailStart->AddComponent<UnityEngine::UI::Text*>();
        botText->set_text(JsonValueToString(bottomPoint.ToJson(allocator)));

        auto trailHandler = trailObject->AddComponent<TrailHandler*>();
        trailObject->AddComponent<TrailComponent*>();

        auto trailEndTransform = trailEnd->AddComponent<TrailTransform*>();
        auto trailStartTransform = trailStart->AddComponent<TrailTransform*>();

        handler->trailHandlers = ArrayW<TrailHandler*>({trailHandler});
        handler->trailTransforms = ArrayW<TrailTransform*>({trailEndTransform, trailStartTransform});

        auto saberModelController = GetComponent<GlobalNamespace::SaberModelController*>();
        auto trail = saberModelController->dyn__saberTrail();
        auto trailRenderer = trail->dyn__trailRenderer();
        auto actualMeshRenderer = trailRenderer->dyn__meshRenderer();
        auto material = actualMeshRenderer->get_material();

        auto meshRenderer = trailObject->AddComponent<UnityEngine::MeshRenderer*>();
        meshRenderer->set_material(material);
    }
}