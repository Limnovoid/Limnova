#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"


namespace YAML
{

    template<>
    struct convert<Limnova::Vector3>
    {
        static Node encode(const Limnova::Vector3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, Limnova::Vector3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();

            return true;
        }
    };


    template<>
    struct convert<Limnova::Vector3d>
    {
        static Node encode(const Limnova::Vector3d& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, Limnova::Vector3d& rhs)
        {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<double>();
            rhs.y = node[1].as<double>();
            rhs.z = node[2].as<double>();

            return true;
        }
    };


    template<>
    struct convert<Limnova::Vector4>
    {
        static Node encode(const Limnova::Vector4& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            return node;
        }

        static bool decode(const Node& node, Limnova::Vector4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();

            return true;
        }
    };


    template<>
    struct convert<Limnova::Quaternion>
    {
        static Node encode(const Limnova::Quaternion& rhs)
        {
            Node node;
            node.push_back(rhs.GetX());
            node.push_back(rhs.GetY());
            node.push_back(rhs.GetZ());
            node.push_back(rhs.GetW());
            return node;
        }

        static bool decode(const Node& node, Limnova::Quaternion& rhs)
        {
            if (!node.IsSequence() || node.size() != 4) {
                return false;
            }

            rhs = Limnova::Quaternion{
                node[0].as<float>(),
                node[1].as<float>(),
                node[2].as<float>(),
                node[3].as<float>()
            };

            return true;
        }
    };

}


namespace Limnova
{

    YAML::Emitter& operator<<(YAML::Emitter& out, const Vector3& vec)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const Vector3d& vec)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const Vector4& vec)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const Quaternion& quat)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << quat.GetX() << quat.GetY() << quat.GetZ() << quat.GetW() << YAML::EndSeq;
        return out;
    }


#ifdef LV_USE_REFLECTION
    namespace Refl = Reflection;

    static void SerializeClass(YAML::Emitter& out, const Refl::Class* classInfo, int8_t* argSrc)
    {
        out << YAML::BeginMap;

        for (const auto& field : classInfo->Fields)
        {
            if (field.Type == nullptr) break;

            out << YAML::Key << field.Name;

            int8_t* source = argSrc + field.Offset;

            switch (field.Type->EnumName)
            {
            case Refl::TypeName::enum_class: {
                SerializeClass(out, field.Type->ClassInfo, source);
            }
            case Refl::TypeName::enum_int8_t: {
                int8_t value = 0;
                memcpy(&value, source, field.Type->Size);
                out << YAML::Value << value;
                break;
            }
            case Refl::TypeName::enum_int16_t:    break;
            case Refl::TypeName::enum_int32_t:    break;
            case Refl::TypeName::enum_uint8_t:    break;
            case Refl::TypeName::enum_uint16_t:   break;
            case Refl::TypeName::enum_uint32_t:   break;
            default:
                assert(false);
                break;
            }
        }

        out << YAML::EndMap;
    }


    template<typename T>
    void SerializeObject(YAML::Emitter& out, T& arg)
    {
        const Refl::Class* classInfo = Refl::GetClass<T>();
        out << YAML::Key << classInfo->Name;
        SerializeClass(out, classInfo, reinterpret_cast<int8_t*>(&arg));
    }
#endif


    void SceneSerializer::SerializeEntity(Scene* scene, YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap; // Entity
        out << YAML::Key << "Entity" << YAML::Value << "0"; /* TODO : entity unique ID */

        if (entity.HasComponent<TagComponent>())
        {
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap; // TagComponent

            auto& tag = entity.GetComponent<TagComponent>();
            out << YAML::Key << "Tag" << YAML::Value << tag.Tag;

            out << YAML::EndMap; // TagComponent
        }

        if (entity.HasComponent<TransformComponent>())
        {
            out << YAML::Key << "TransformComponent";
            out << YAML::BeginMap; // TransformComponent

            auto& transform = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "Position" << YAML::Value << transform.GetPosition();
            out << YAML::Key << "Orientation" << YAML::Value << transform.GetOrientation();
            out << YAML::Key << "EulerAngles" << YAML::Value << transform.GetEulerAngles();
            out << YAML::Key << "Scale" << YAML::Value << transform.GetScale();

            out << YAML::EndMap; // TransformComponent
        }

        if (entity.HasComponent<HierarchyComponent>())
        {
            out << YAML::Key << "HierarchyComponent";
            out << YAML::BeginMap; // HierarchyComponent

            auto& hierarchy = entity.GetComponent<HierarchyComponent>();
            out << YAML::Key << "Parent" << YAML::Value << (uint32_t)0; /* TODO : entity unique ID */
            //out << YAML::Key << "NextSibling" << YAML::Value << (uint32_t)0; /* TODO : entity unique ID */
            //out << YAML::Key << "PrevSibling" << YAML::Value << (uint32_t)0; /* TODO : entity unique ID */
            //out << YAML::Key << "FirstChild" << YAML::Value << (uint32_t)0; /* TODO : entity unique ID */

            out << YAML::EndMap; // HierarchyComponent
        }

        if (entity.HasComponent<CameraComponent>())
        {
            out << YAML::Key << "CameraComponent";
            out << YAML::BeginMap; // CameraComponent

            auto& camera = entity.GetComponent<CameraComponent>();
            out << YAML::Key << "VerticalFov" << YAML::Value << camera.VerticalFov;
            out << YAML::Key << "OrthographicHeight" << YAML::Value << camera.OrthographicHeight;
            out << YAML::Key << "AspectRatio" << YAML::Value << camera.AspectRatio;
            out << YAML::Key << "OrthoNearClip" << YAML::Value << camera.OrthoNearClip;
            out << YAML::Key << "PerspNearClip" << YAML::Value << camera.PerspNearClip;
            out << YAML::Key << "TieAspectToView" << YAML::Value << camera.TieAspectToView;
            out << YAML::Key << "IsOrthographic" << YAML::Value << camera.IsOrthographic;

            out << YAML::EndMap; // CameraComponent
        }

        if (entity.HasComponent<NativeScriptComponent>())
        {
            out << YAML::Key << "NativeScriptComponent";
            out << YAML::BeginMap; // NativeScriptComponent

            auto& script = entity.GetComponent<NativeScriptComponent>();
            out << YAML::Key << "Script" << YAML::Value << 0; /* TODO : asset unique ID */

            out << YAML::EndMap; // NativeScriptComponent
        }

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            out << YAML::Key << "SpriteRendererComponent";
            out << YAML::BeginMap; // SpriteRendererComponent

            auto& sprite = entity.GetComponent<SpriteRendererComponent>();
            out << YAML::Key << "Color" << YAML::Value << sprite.Color;

            out << YAML::EndMap; // SpriteRendererComponent
        }

        if (entity.HasComponent<CircleRendererComponent>())
        {
            out << YAML::Key << "CircleRendererComponent";
            out << YAML::BeginMap; // CircleRendererComponent

            auto& sprite = entity.GetComponent<CircleRendererComponent>();
            out << YAML::Key << "Color" << YAML::Value << sprite.Color;
            out << YAML::Key << "Thickness" << YAML::Value << sprite.Thickness;
            out << YAML::Key << "Fade" << YAML::Value << sprite.Fade;

            out << YAML::EndMap; // CircleRendererComponent
        }

        if (entity.HasComponent<EllipseRendererComponent>())
        {
            out << YAML::Key << "EllipseRendererComponent";
            out << YAML::BeginMap; // EllipseRendererComponent

            auto& sprite = entity.GetComponent<EllipseRendererComponent>();
            out << YAML::Key << "Color" << YAML::Value << sprite.Color;
            out << YAML::Key << "Thickness" << YAML::Value << sprite.Thickness;
            out << YAML::Key << "Fade" << YAML::Value << sprite.Fade;

            out << YAML::EndMap; // EllipseRendererComponent
        }

        if (entity.HasComponent<OrbitalComponent>())
        {
            out << YAML::Key << "OrbitalComponent";
            out << YAML::BeginMap; // OrbitalComponent

            auto& orbital = entity.GetComponent<OrbitalComponent>();
            out << YAML::Key << "LocalScale" << YAML::Value << orbital.LocalScale;
            out << YAML::Key << "Albedo" << YAML::Value << orbital.Albedo;

            out << YAML::Key << "Mass" << YAML::Value << orbital.GetMass();
            out << YAML::Key << "Position" << YAML::Value << orbital.GetPosition();
            out << YAML::Key << "Velocity" << YAML::Value << orbital.GetVelocity();
            out << YAML::Key << "LocalSpaceRadius" << YAML::Value << orbital.GetLocalSpaceRadius();
            

            out << YAML::EndMap; // OrbitalComponent
        }

        out << YAML::EndMap; // Entity
    }


    void SceneSerializer::DeserializeEntity(Scene* scene, YAML::Node entityNode)
    {
        uint64_t uuid = entityNode["Entity"].as<uint64_t>(); // TODO : universal unique identifier (uuid)

        std::string name;
        if (auto tagComponent = entityNode["TagComponent"]) {
            name = tagComponent["Tag"].as<std::string>();
        }
        LV_CORE_TRACE("Deserializing entity '{0}'", name);

        Entity entity = (name == "Root") ? scene->GetRoot()
            : scene->CreateEntity(name); // TODO : deserialize with uuid

        if (auto tcNode = entityNode["TransformComponent"])
        {
            /* Scene gives every created entity a transform component */
            auto& tc = entity.GetComponent<TransformComponent>();
            tc.SetPosition(tcNode["Position"].as<Vector3>());
            tc.SetOrientation(tcNode["Orientation"].as<Quaternion>());
            tc.SetEulerAngles(tcNode["EulerAngles"].as<Vector3>());
            tc.SetScale(tcNode["Scale"].as<Vector3>());
        }

        if (auto hcNode = entityNode["HierarchyComponent"])
        {
            /* Scene gives every created entity a hierarchy component */
            auto& hc = entity.GetComponent<HierarchyComponent>();
            //entity.Parent(Entity{ (entt::entity)hcNode["Parent"].as<uint32_t>(), m_Scene.get() }); /* TODO - parent with uuid */
        }

        if (auto ccNode = entityNode["CameraComponent"])
        {
            auto& cc = entity.AddComponent<CameraComponent>();

            cc.VerticalFov          = ccNode["VerticalFov"].as<float>();
            cc.OrthographicHeight   = ccNode["OrthographicHeight"].as<float>();
            cc.OrthoNearClip        = ccNode["OrthoNearClip"].as<float>();
            cc.PerspNearClip        = ccNode["PerspNearClip"].as<float>();
            cc.TieAspectToView      = ccNode["TieAspectToView"].as<bool>();
            cc.IsOrthographic       = ccNode["IsOrthographic"].as<bool>();

            cc.AspectRatio = cc.TieAspectToView
                ? scene->m_ViewportAspectRatio
                : ccNode["AspectRatio"].as<float>();

            cc.UpdateProjection();
        }

        if (auto nscNode = entityNode["NativeScriptComponent"])
        {
            /* Scene gives every created entity a hierarchy component */
            auto& nsc = entity.AddComponent<NativeScriptComponent>();
            //nsc.Bind(nscNode["Script"]); // TODO - use asset ID to bind script
        }

        if (auto srcNode = entityNode["SpriteRendererComponent"])
        {
            auto& src = entity.AddComponent<SpriteRendererComponent>();

            src.Color = srcNode["Color"].as<Vector4>();
        }

        if (auto crcNode = entityNode["CircleRendererComponent"])
        {
            auto& crc = entity.AddComponent<CircleRendererComponent>();

            crc.Color       = crcNode["Color"].as<Vector4>();
            crc.Thickness   = crcNode["Thickness"].as<float>();
            crc.Fade        = crcNode["Fade"].as<float>();
        }

        if (auto ercNode = entityNode["EllipseRendererComponent"])
        {
            auto& erc = entity.AddComponent<EllipseRendererComponent>();

            erc.Color       = ercNode["Color"].as<Vector4>();
            erc.Thickness   = ercNode["Thickness"].as<float>();
            erc.Fade        = ercNode["Fade"].as<float>();
        }

        if (auto oNode = entityNode["OrbitalComponent"])
        {
            auto& oc = entity.AddComponent<OrbitalComponent>();

            oc.LocalScale   = oNode["LocalScale"].as<float>();
            oc.Albedo       = oNode["Albedo"].as<float>();

            oc.SetMass(             oNode["Mass"].as<double>());
            oc.SetPosition(         oNode["Position"].as<Vector3>());
            oc.SetVelocity(         oNode["Velocity"].as<Vector3d>());
            oc.SetLocalSpaceRadius( oNode["LocalSpaceRadius"].as<float>());
        }

    }



#ifdef EXCLUDE
    void SceneSerializer::Serialize(const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap; // Scene
        out << YAML::Key << "Scene" << YAML::Value << "Untitled";
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        // TODO - serialise in descending scene hierarchy order,
        // so each entity is deserialised after its parent and can be immediately parented
        m_Scene->m_Registry.each([&](auto entityId)
        {
            Entity entity = { entityId, m_Scene.get() };
            if (!entity) {
                return;
            }

            SerializeEntity(out, entity);
        });
        out << YAML::EndSeq;
        out << YAML::EndMap; // Scene

        std::ofstream fout(filepath);
        fout << out.c_str();
    }


    void SceneSerializer::SerializeRuntime(const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
    }


    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        std::ifstream ifile(filepath);
        std::stringstream sstream;
        sstream << ifile.rdbuf();

        YAML::Node data = YAML::Load(sstream.str());
        if (!data["Scene"]) {
            return false;
        }

        std::string sceneName = data["Scene"].as<std::string>();
        LV_CORE_TRACE("Deserializing scene '{0}'", sceneName);

        YAML::Node entitiesNode = data["Entities"];
        if (entitiesNode)
        {
            for (YAML::Node entityNode : entitiesNode)
            {
                DeserializeEntity(entityNode);
            }
        }
    }


    bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
        return false;
    }


    // OrbitalScene ////////////////////////////////////////////////////////////////

    void OrbitalSceneSerializer::Serialize(const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap; // Scene
        out << YAML::Key << "OrbitalScene" << YAML::Value << "Untitled";

        out << YAML::Key << "InfluenceColor"        << YAML::Value << m_Scene->m_InfluenceColor;
        out << YAML::Key << "InfluenceThickness"    << YAML::Value << m_Scene->m_InfluenceThickness;
        out << YAML::Key << "InfluenceFade"         << YAML::Value << m_Scene->m_InfluenceFade;
        out << YAML::Key << "OrbitThickness"        << YAML::Value << m_Scene->m_OrbitThickness;
        out << YAML::Key << "OrbitThicknessFactor"  << YAML::Value << m_Scene->m_OrbitThicknessFactor;
        out << YAML::Key << "OrbitAlpha"            << YAML::Value << m_Scene->m_OrbitAlpha;
        //out << YAML::Key << "ViewPrimary" << YAML::Value << (uint32_t)(m_Scene->m_ViewPrimary); // TODO - use uuid

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        // TODO - serialise in descending scene hierarchy order,
        // so each entity is deserialised after its parent and can be immediately parented
        m_Scene->m_Registry.each([&](auto entityId)
        {
            Entity entity = { entityId, m_Scene.get() };
            if (!entity) {
                return;
            }

            SerializeEntity(out, entity);
        });
        out << YAML::EndSeq;
        out << YAML::EndMap; // Scene

        std::ofstream fout(filepath);
        fout << out.c_str();
    }


    void OrbitalSceneSerializer::SerializeRuntime(const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
    }


    bool OrbitalSceneSerializer::Deserialize(const std::string& filepath)
    {
        std::ifstream ifile(filepath);
        std::stringstream sstream;
        sstream << ifile.rdbuf();

        YAML::Node data = YAML::Load(sstream.str());
        if (!data["OrbitalScene"]) {
            return false;
        }
        std::string sceneName = data["OrbitalScene"].as<std::string>();
        LV_CORE_TRACE("Deserializing orbital scene '{0}'", sceneName);

        m_Scene->m_InfluenceColor       = data["InfluenceColor"].as<Vector4>();
        m_Scene->m_InfluenceThickness   = data["InfluenceThickness"].as<float>();
        m_Scene->m_InfluenceFade        = data["InfluenceFade"].as<float>();
        m_Scene->m_OrbitThickness       = data["OrbitThickness"].as<float>();
        m_Scene->m_OrbitThicknessFactor = data["OrbitThicknessFactor"].as<float>();
        m_Scene->m_OrbitAlpha           = data["OrbitAlpha"].as<float>();
        //m_Scene->m_ViewPrimary          = data["ViewPrimary"].as<uint32_t>(); // TODO - use uuid

        YAML::Node entitiesNode = data["Entities"];
        if (entitiesNode)
        {
            for (YAML::Node entityNode : entitiesNode)
            {
                DeserializeEntity(entityNode);
            }
        }
    }


    bool OrbitalSceneSerializer::DeserializeRuntime(const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
        return false;
    }
#endif


    // Scene ///////////////////////////////////////////////////////////////////

    void SceneSerializer::Serialize(Scene* scene, const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap; // Scene
        out << YAML::Key << "Scene" << YAML::Value << "Untitled";
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        // TODO - serialise in descending scene hierarchy order,
        // so each entity is deserialised after its parent and can be immediately parented
        scene->m_Registry.each([&](auto entityId)
            {
                Entity entity = { entityId, scene };
                if (!entity) {
                    return;
                }

                SerializeEntity(scene, out, entity);
            });
        out << YAML::EndSeq;
        out << YAML::EndMap; // Scene

        std::ofstream fout(filepath);
        fout << out.c_str();
    }


    void SceneSerializer::SerializeRuntime(Scene* scene, const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
    }


    bool SceneSerializer::Deserialize(Scene* scene, const std::string& filepath)
    {
        std::ifstream ifile(filepath);
        std::stringstream sstream;
        sstream << ifile.rdbuf();

        YAML::Node data = YAML::Load(sstream.str());
        if (!data["Scene"]) {
            return false;
        }

        std::string sceneName = data["Scene"].as<std::string>();
        LV_CORE_TRACE("Deserializing scene '{0}'", sceneName);

        YAML::Node entitiesNode = data["Entities"];
        if (entitiesNode)
        {
            for (YAML::Node entityNode : entitiesNode)
            {
                DeserializeEntity(scene, entityNode);
            }
        }

        return true;
    }


    bool SceneSerializer::DeserializeRuntime(Scene* scene, const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
        return false;
    }


    // OrbitalScene ////////////////////////////////////////////////////////////

    void SceneSerializer::Serialize(OrbitalScene* scene, const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap; // Scene
        out << YAML::Key << "OrbitalScene" << YAML::Value << "Untitled";

        out << YAML::Key << "InfluenceColor"        << YAML::Value << scene->m_InfluenceColor;
        out << YAML::Key << "InfluenceThickness"    << YAML::Value << scene->m_InfluenceThickness;
        out << YAML::Key << "InfluenceFade"         << YAML::Value << scene->m_InfluenceFade;
        out << YAML::Key << "OrbitThickness"        << YAML::Value << scene->m_OrbitThickness;
        out << YAML::Key << "OrbitThicknessFactor"  << YAML::Value << scene->m_OrbitThicknessFactor;
        out << YAML::Key << "OrbitAlpha"            << YAML::Value << scene->m_OrbitAlpha;
        //out << YAML::Key << "ViewPrimary" << YAML::Value << (uint32_t)(scene->m_ViewPrimary); // TODO - use uuid

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        // TODO - serialise in descending scene hierarchy order,
        // so each entity is deserialised after its parent and can be immediately parented
        scene->m_Registry.each([&](auto entityId)
        {
            Entity entity = { entityId, scene };
            if (!entity) {
                return;
            }

            SerializeEntity(scene, out, entity);
        });
        out << YAML::EndSeq;
        out << YAML::EndMap; // Scene

        std::ofstream fout(filepath);
        fout << out.c_str();
    }


    void SceneSerializer::SerializeRuntime(OrbitalScene* scene, const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
    }


    bool SceneSerializer::Deserialize(OrbitalScene* scene, const std::string& filepath)
    {
        std::ifstream ifile(filepath);
        std::stringstream sstream;
        sstream << ifile.rdbuf();

        YAML::Node data = YAML::Load(sstream.str());
        if (!data["OrbitalScene"]) {
            return false;
        }

        std::string sceneName = data["OrbitalScene"].as<std::string>();
        LV_CORE_TRACE("Deserializing orbital scene '{0}'", sceneName);

        scene->m_InfluenceColor = data["InfluenceColor"].as<Vector4>();
        scene->m_InfluenceThickness = data["InfluenceThickness"].as<float>();
        scene->m_InfluenceFade = data["InfluenceFade"].as<float>();
        scene->m_OrbitThickness = data["OrbitThickness"].as<float>();
        scene->m_OrbitThicknessFactor = data["OrbitThicknessFactor"].as<float>();
        scene->m_OrbitAlpha = data["OrbitAlpha"].as<float>();
        //m_Scene->m_ViewPrimary          = data["ViewPrimary"].as<uint32_t>(); // TODO - use uuid

        YAML::Node entitiesNode = data["Entities"];
        if (entitiesNode)
        {
            for (YAML::Node entityNode : entitiesNode)
            {
                DeserializeEntity(scene, entityNode);
            }
        }

        return true;
    }


    bool SceneSerializer::DeserializeRuntime(OrbitalScene* scene, const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
        return false;
    }

}
