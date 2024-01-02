#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"


namespace YAML
{

    template<>
    struct convert<Limnova::UUID>
    {
        static Node encode(const Limnova::UUID rhs)
        {
            Node node;
            node.push_back((uint64_t)rhs);
            return node;
        }

        static bool decode(const Node& node, Limnova::UUID &rhs)
        {
            if (!node.IsScalar())
                return false;

            rhs = node.as<uint64_t>();
            return true;
        }
    };

    // -----------------------------------------------------------------------------------------------------------------------------

    template<>
    struct convert<Limnova::Vector2>
    {
        static Node encode(const Limnova::Vector2& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            return node;
        }

        static bool decode(const Node& node, Limnova::Vector2& rhs)
        {
            if (!node.IsSequence() || node.size() != 2)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();

            return true;
        }
    };

    // -----------------------------------------------------------------------------------------------------------------------------

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
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();

            return true;
        }
    };

    // -----------------------------------------------------------------------------------------------------------------------------

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
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<double>();
            rhs.y = node[1].as<double>();
            rhs.z = node[2].as<double>();

            return true;
        }
    };

    // -----------------------------------------------------------------------------------------------------------------------------

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
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();

            return true;
        }
    };

    // -----------------------------------------------------------------------------------------------------------------------------

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
            if (!node.IsSequence() || node.size() != 4)
                return false;

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

#define LV_YAML_SERIALIZE_NODE(outstream, stringkey, value)\
outstream << YAML::Key << stringkey << YAML::Value << value

    // -----------------------------------------------------------------------------------------------------------------------------

#define LV_YAML_DESERIALIZE_NODE(data, name, type, variable)        \
if (auto node = data[name])                                         \
{                                                                   \
    variable = node.as<type>();                                     \
}                                                                   \
else                                                                \
{                                                                   \
    LV_CORE_ERROR("Failed to deserialize node: " name);             \
}

    // -----------------------------------------------------------------------------------------------------------------------------

#define LV_YAML_DESERIALIZE_NODE_STRING_CLASS(data, nameObject, type, variable)     \
if (auto node = data[nameObject])                                                   \
{                                                                                   \
    variable = node.as<type>();                                                     \
}                                                                                   \
else                                                                                \
{                                                                                   \
    LV_CORE_ERROR("Failed to deserialize node: {}", nameObject);                    \
}

    // -----------------------------------------------------------------------------------------------------------------------------

#define LV_YAML_DESERIALIZE_NODE_WITH_SETTER(data, stringname, type, setter)    \
if (auto node = data[stringname])                                               \
{                                                                               \
    setter(node.as<type>());                                                    \
}                                                                               \
else                                                                            \
{                                                                               \
    LV_CORE_ERROR("Failed to deserialize node: " stringname);                   \
}

    // -----------------------------------------------------------------------------------------------------------------------------

//#define LV_YAML_SERIALIZE_SCRIPT_FIELD(id, type, name, monoTypeName)    \
    case ScriptEngine::id: { type value; field.second->GetValue(value); \
    LV_YAML_SERIALIZE_NODE(out, field.first, value); break; }
#define LV_YAML_SERIALIZE_SCRIPT_FIELD(id, type, name, monoTypeName)\
    case ScriptEngine::id: { type value; field.second->GetValue(value); out << YAML::Value << value; break; }

    // -----------------------------------------------------------------------------------------------------------------------------

//#define LV_YAML_DESERIALIZE_SCRIPT_FIELD(id, type, name, monoTypeName)                  \
case ScriptEngine::id: { type value;                                                    \
    LV_YAML_DESERIALIZE_NODE_STRING_CLASS(scriptFieldsNode, field.first, type, value);  \
    field.second->SetValue<type>(value); break; }
//#define LV_YAML_DESERIALIZE_SCRIPT_FIELD(id, type, name, monoTypeName)  \
    case ScriptEngine::id: { type value = fieldNode["Data"].as<type>();  \
    field.second->SetValue<type>(value); break; }
#define LV_YAML_DESERIALIZE_SCRIPT_FIELD(id, type, name, monoTypeName)  \
    case ScriptEngine::id: { type value;\
    if (auto dataNode = fieldNode["Data"]) { value = dataNode.as<type>(); field.second->SetValue<type>(value); isFieldDeserialized = true; }\
    break; }

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    YAML::Emitter& operator<<(YAML::Emitter& out, const Vector2& vec)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
        return out;
    }

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

    // -----------------------------------------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------------------------------------

    void SceneSerializer::SerializeEntity(Scene* scene, YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap; // Entity

        UUID uuid = entity.GetUUID();

        LV_YAML_SERIALIZE_NODE(out, "Entity", (uint64_t)uuid);

        bool isRootEntity;
        if (entity.HasComponent<TagComponent>())
        {
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap; // TagComponent

            auto& tag = entity.GetComponent<TagComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Tag", tag.Tag);

            out << YAML::EndMap; // TagComponent

            isRootEntity = tag.Tag == "Root";
        }

        if (entity.HasComponent<HierarchyComponent>())
        {
            out << YAML::Key << "HierarchyComponent";
            out << YAML::BeginMap; // HierarchyComponent

            auto& hc = entity.GetComponent<HierarchyComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Parent", (uint64_t)hc.Parent);

            /* Only store the parent: all other relationships (siblings/children) are implicitly serialised
             * by those entities which share this entity's parent or have this entity as their parent */

            out << YAML::EndMap; // HierarchyComponent
        }

        if (entity.HasComponent<TransformComponent>())
        {
            out << YAML::Key << "TransformComponent";
            out << YAML::BeginMap; // TransformComponent

            auto& transform = entity.GetComponent<TransformComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Position",     transform.GetPosition());
            LV_YAML_SERIALIZE_NODE(out, "Orientation",  transform.GetOrientation());
            LV_YAML_SERIALIZE_NODE(out, "EulerAngles",  transform.GetEulerAngles());
            LV_YAML_SERIALIZE_NODE(out, "Scale",        transform.GetScale());

            out << YAML::EndMap; // TransformComponent
        }

        if (entity.HasComponent<ScriptComponent>())
        {
            out << YAML::Key << "ScriptComponent";
            out << YAML::BeginMap; // ScriptComponent

            auto& script = entity.GetComponent<ScriptComponent>();
            if (script.HasInstance())
            {
                LV_YAML_SERIALIZE_NODE(out, "Name", script.GetScriptName());

                out << YAML::Key << "Fields";
                out << YAML::BeginMap; // Script fields

                auto &fields = script.GetScriptInstance(uuid)->GetFields();
                for (auto &field : fields)
                {
                    out << YAML::Key << field.first;
                    out << YAML::BeginMap; // Field
                    out << YAML::Key << "Type" << YAML::Value << ScriptEngine::FieldTypeToString(field.second->GetType());
                    out << YAML::Key << "Data";
                    switch (field.second->GetType())
                    {
                        LV_SCRIPT_ENGINE_FIELD_LIST(LV_YAML_SERIALIZE_SCRIPT_FIELD);
                    }
                    out << YAML::EndMap; // Field
                }

                out << YAML::EndMap; // Script fields
            }
            out << YAML::EndMap; // ScriptComponent
        }

        if (entity.HasComponent<CameraComponent>())
        {
            out << YAML::Key << "CameraComponent";
            out << YAML::BeginMap; // CameraComponent

            auto& camera = entity.GetComponent<CameraComponent>();
            LV_YAML_SERIALIZE_NODE(out, "VerticalFov",          camera.VerticalFov);
            LV_YAML_SERIALIZE_NODE(out, "OrthographicHeight",   camera.OrthographicHeight);
            LV_YAML_SERIALIZE_NODE(out, "AspectRatio",          camera.AspectRatio);
            LV_YAML_SERIALIZE_NODE(out, "OrthoNearClip",        camera.OrthoNearClip);
            LV_YAML_SERIALIZE_NODE(out, "OrthoFarClip",         camera.OrthoFarClip);
            LV_YAML_SERIALIZE_NODE(out, "PerspNearClip",        camera.PerspNearClip);
            LV_YAML_SERIALIZE_NODE(out, "PerspFarClip",         camera.PerspFarClip);
            LV_YAML_SERIALIZE_NODE(out, "TieAspectToView",      camera.TieAspectToView);
            LV_YAML_SERIALIZE_NODE(out, "IsOrthographic",       camera.IsOrthographic);

            out << YAML::EndMap; // CameraComponent
        }

        if (entity.HasComponent<NativeScriptComponent>())
        {
            out << YAML::Key << "NativeScriptComponent";
            out << YAML::BeginMap; // NativeScriptComponent

            auto& script = entity.GetComponent<NativeScriptComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Script", 0); /* TODO : use asset ID */

            out << YAML::EndMap; // NativeScriptComponent
        }

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            out << YAML::Key << "SpriteRendererComponent";
            out << YAML::BeginMap; // SpriteRendererComponent

            auto& src = entity.GetComponent<SpriteRendererComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Color", src.Color);

            out << YAML::EndMap; // SpriteRendererComponent
        }

        if (entity.HasComponent<BillboardSpriteRendererComponent>())
        {
            out << YAML::Key << "BillboardSpriteRendererComponent";
            out << YAML::BeginMap; // BillboardSpriteRendererComponent

            auto& bsrc = entity.GetComponent<BillboardSpriteRendererComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Color", bsrc.Color);

            out << YAML::EndMap; // BillboardSpriteRendererComponent
        }

        if (entity.HasComponent<CircleRendererComponent>())
        {
            out << YAML::Key << "CircleRendererComponent";
            out << YAML::BeginMap; // CircleRendererComponent

            auto& crc = entity.GetComponent<CircleRendererComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Color",        crc.Color);
            LV_YAML_SERIALIZE_NODE(out, "Thickness",    crc.Thickness);
            LV_YAML_SERIALIZE_NODE(out, "Fade",         crc.Fade);

            out << YAML::EndMap; // CircleRendererComponent
        }

        if (entity.HasComponent<BillboardCircleRendererComponent>())
        {
            out << YAML::Key << "BillboardCircleRendererComponent";
            out << YAML::BeginMap; // BillboardCircleRendererComponent

            auto& bcrc = entity.GetComponent<BillboardCircleRendererComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Color",        bcrc.Color);
            LV_YAML_SERIALIZE_NODE(out, "Thickness",    bcrc.Thickness);
            LV_YAML_SERIALIZE_NODE(out, "Fade",         bcrc.Fade);

            out << YAML::EndMap; // BillboardCircleRendererComponent
        }

        if (entity.HasComponent<EllipseRendererComponent>())
        {
            out << YAML::Key << "EllipseRendererComponent";
            out << YAML::BeginMap; // EllipseRendererComponent

            auto& erc = entity.GetComponent<EllipseRendererComponent>();
            LV_YAML_SERIALIZE_NODE(out, "Color",        erc.Color);
            LV_YAML_SERIALIZE_NODE(out, "Thickness",    erc.Thickness);
            LV_YAML_SERIALIZE_NODE(out, "Fade",         erc.Fade);

            out << YAML::EndMap; // EllipseRendererComponent
        }

        if (entity.HasComponent<OrbitalHierarchyComponent>())
        {
            out << YAML::Key << "OrbitalHierarchyComponent";
            out << YAML::BeginMap; // OrbitalHierarchyComponent

            auto& ohc = entity.GetComponent<OrbitalHierarchyComponent>();
            LV_YAML_SERIALIZE_NODE(out, "AbsoluteScale",    ohc.AbsoluteScale);
            LV_YAML_SERIALIZE_NODE(out, "LocalSpace",       ohc.LocalSpaceRelativeToParent);

            out << YAML::EndMap; // OrbitalHierarchyComponent
        }

        if (entity.HasComponent<OrbitalComponent>())
        {
            out << YAML::Key << "OrbitalComponent";
            out << YAML::BeginMap; // OrbitalComponent

            auto& orbital = entity.GetComponent<OrbitalComponent>();
            LV_YAML_SERIALIZE_NODE(out, "UIColor",              orbital.UIColor);
            LV_YAML_SERIALIZE_NODE(out, "Albedo",               orbital.Albedo);
            LV_YAML_SERIALIZE_NODE(out, "ShowMajorMinorAxes",   orbital.ShowMajorMinorAxes);
            LV_YAML_SERIALIZE_NODE(out, "ShowNormal",           orbital.ShowNormal);

            out << YAML::Key << "Mass"                  << YAML::Value << orbital.Object.GetState().Mass;
            if (!isRootEntity)
            {
                LV_YAML_SERIALIZE_NODE(out, "Position", orbital.Object.GetState().Position);
                LV_YAML_SERIALIZE_NODE(out, "Velocity", orbital.Object.GetState().Velocity);
            }
            if (orbital.Object.IsDynamic())
                LV_YAML_SERIALIZE_NODE(out, "ContAcceleration", orbital.Object.GetDynamics().ContAcceleration);


            out << YAML::Key << "LocalSpaceRadii" << YAML::BeginSeq;
            for (auto lspNode : orbital.LocalSpaces)
            {
                if (isRootEntity && lspNode.IsRoot()) continue; /* no entry for root local space - root objects are created by Context constructor so we don't want to duplicate them in deserialisation! */
                if (!lspNode.IsSphereOfInfluence()) {
                    out << lspNode.GetLSpace().Radius;
                }
            }
            out << YAML::EndSeq;

            out << YAML::EndMap; // OrbitalComponent
        }

        out << YAML::EndMap; // Entity
    }


    void SceneSerializer::DeserializeEntity(Scene* scene, YAML::Node entityNode)
    {
        uint64_t uuid = UUID::Null;
        LV_YAML_DESERIALIZE_NODE(entityNode, "Entity", uint64_t, uuid);

        std::string name;
        if (auto tagComponent = entityNode["TagComponent"])
            LV_YAML_DESERIALIZE_NODE(tagComponent, "Tag", std::string, name);

        LV_CORE_TRACE("Deserializing entity '{0}'", name);

        UUID parentId = UUID::Null;
        if (auto hcNode = entityNode["HierarchyComponent"])
            LV_YAML_DESERIALIZE_NODE(hcNode, "Parent", uint64_t, parentId);

        bool isRootEntity = parentId == UUID::Null;

        Entity entity;
        if (isRootEntity)
        {
            entity = scene->GetRoot();
            scene->SetRootId(uuid);
        }
        else {
            entity = scene->CreateEntityFromUUID(uuid, name, parentId);
        }

        if (auto tcNode = entityNode["TransformComponent"])
        {
            /* Scene gives every created entity a transform component */
            auto& tc = entity.GetComponent<TransformComponent>();
            LV_YAML_DESERIALIZE_NODE_WITH_SETTER(tcNode, "Position",    Vector3, tc.SetPosition);
            LV_YAML_DESERIALIZE_NODE_WITH_SETTER(tcNode, "Orientation", Quaternion, tc.SetOrientation);
            LV_YAML_DESERIALIZE_NODE_WITH_SETTER(tcNode, "EulerAngles", Vector3, tc.SetEulerAngles);
            LV_YAML_DESERIALIZE_NODE_WITH_SETTER(tcNode, "Scale",       Vector3, tc.SetScale);
        }

        if (auto sNode = entityNode["ScriptComponent"])
        {
            auto& sc = entity.AddComponent<ScriptComponent>();

            if (auto scriptNameNode = sNode["Name"])
            {
                if (!sc.SetScript(uuid, scriptNameNode.as<std::string>()))
                {
                    LV_CORE_ERROR("Failed to deserialize ScriptComponent: unrecognised entity script '{0}'", scriptNameNode.as<std::string>());
                }
                else
                {
                    if (auto scriptFieldsNode = sNode["Fields"];
                        scriptFieldsNode && scriptFieldsNode.IsMap())
                    {
                        auto &fields = sc.GetScriptInstance(uuid)->GetFields();
                        for (auto &field : fields)
                        {
                            bool isFieldDeserialized = false;

                            if (auto fieldNode = scriptFieldsNode[field.first];
                                fieldNode && fieldNode.IsMap())
                            {
                                auto fieldTypeNode = fieldNode["Type"];
                                if (fieldTypeNode && fieldTypeNode.IsScalar() &&
                                    fieldTypeNode.as<std::string>() == ScriptEngine::FieldTypeToString(field.second->GetType()))
                                {
                                    switch (field.second->GetType())
                                    {
                                    LV_SCRIPT_ENGINE_FIELD_LIST(LV_YAML_DESERIALIZE_SCRIPT_FIELD)
                                    }
                                }
                            }

                            if (!isFieldDeserialized)
                                LV_CORE_ERROR("Failed to deserialize script field '{}'", field.first);
                        }
                    }
                }
            }
        }

        if (auto ccNode = entityNode["CameraComponent"])
        {
            auto& cc = entity.AddComponent<CameraComponent>();

            LV_YAML_DESERIALIZE_NODE(ccNode, "VerticalFov",         float,  cc.VerticalFov          );
            LV_YAML_DESERIALIZE_NODE(ccNode, "OrthographicHeight",  float,  cc.OrthographicHeight   );
            LV_YAML_DESERIALIZE_NODE(ccNode, "OrthoNearClip",       float,  cc.OrthoNearClip        );
            LV_YAML_DESERIALIZE_NODE(ccNode, "OrthoFarClip",        float,  cc.OrthoFarClip         );
            LV_YAML_DESERIALIZE_NODE(ccNode, "PerspNearClip",       float,  cc.PerspNearClip        );
            LV_YAML_DESERIALIZE_NODE(ccNode, "PerspFarClip",        float,  cc.PerspFarClip         );
            LV_YAML_DESERIALIZE_NODE(ccNode, "TieAspectToView",     bool,   cc.TieAspectToView      );
            LV_YAML_DESERIALIZE_NODE(ccNode, "IsOrthographic",      bool,   cc.IsOrthographic       );

            cc.AspectRatio = (cc.TieAspectToView) ?
                scene->m_ViewportAspectRatio : ccNode["AspectRatio"].as<float>();

            cc.UpdateProjection();
        }

        if (auto nscNode = entityNode["NativeScriptComponent"])
        {
            auto& nsc = entity.AddComponent<NativeScriptComponent>();
            //nsc.Bind(nscNode["Script"].as<>); // TODO - use asset ID to bind script
        }

        if (auto srcNode = entityNode["SpriteRendererComponent"])
        {
            auto& src = entity.AddComponent<SpriteRendererComponent>();
            LV_YAML_DESERIALIZE_NODE(srcNode, "Color", Vector4, src.Color);
        }

        if (auto bsrcNode = entityNode["BillboardSpriteRendererComponent"])
        {
            auto& bsrc = entity.AddComponent<BillboardSpriteRendererComponent>();
            LV_YAML_DESERIALIZE_NODE(bsrcNode, "Color", Vector4, bsrc.Color);
        }

        if (auto crcNode = entityNode["CircleRendererComponent"])
        {
            auto& crc = entity.AddComponent<CircleRendererComponent>();
            LV_YAML_DESERIALIZE_NODE(crcNode, "Color",      Vector4,    crc.Color       );
            LV_YAML_DESERIALIZE_NODE(crcNode, "Thickness",  float,      crc.Thickness   );
            LV_YAML_DESERIALIZE_NODE(crcNode, "Fade",       float,      crc.Fade        );
        }

        if (auto bcrcNode = entityNode["BillboardCircleRendererComponent"])
        {
            auto& crc = entity.AddComponent<BillboardCircleRendererComponent>();
            LV_YAML_DESERIALIZE_NODE(bcrcNode,  "Color",        Vector4,    crc.Color);
            LV_YAML_DESERIALIZE_NODE(bcrcNode,  "Thickness",    float,      crc.Thickness);
            LV_YAML_DESERIALIZE_NODE(bcrcNode,  "Fade",         float,      crc.Fade);
        }

        if (auto ercNode = entityNode["EllipseRendererComponent"])
        {
            auto& erc = entity.AddComponent<EllipseRendererComponent>();
            LV_YAML_DESERIALIZE_NODE(ercNode,   "Color",        Vector4,    erc.Color);
            LV_YAML_DESERIALIZE_NODE(ercNode,   "Thickness",    float,      erc.Thickness);
            LV_YAML_DESERIALIZE_NODE(ercNode,   "Fade",         float,      erc.Fade);
        }

        if (auto ohcNode = entityNode["OrbitalHierarchyComponent"])
        {
            auto& ohc = entity.GetComponent<OrbitalHierarchyComponent>();
            LV_YAML_DESERIALIZE_NODE(ohcNode, "AbsoluteScale",  Vector3d,   ohc.AbsoluteScale               );
            LV_YAML_DESERIALIZE_NODE(ohcNode, "LocalSpace",     int,        ohc.LocalSpaceRelativeToParent  );
        }

        if (auto oNode = entityNode["OrbitalComponent"])
        {
            auto& oc = isRootEntity
                ? entity.GetComponent<OrbitalComponent>()
                : entity.AddComponent<OrbitalComponent>();

            if (!isRootEntity)
            {
                LV_YAML_DESERIALIZE_NODE_WITH_SETTER(oNode, "Position", Vector3,    oc.Object.SetPosition);
                LV_YAML_DESERIALIZE_NODE_WITH_SETTER(oNode, "Velocity", Vector3d,   oc.Object.SetVelocity);
            }
            oc.Object.SetMass(             oNode["Mass"].as<double>());

            if (auto contAcceleration = oNode["ContAcceleration"])
            {
                oc.Object.SetDynamic(true);
                oc.Object.SetContinuousAcceleration(contAcceleration.as<Vector3d>());
            }

            auto localSpaceRadiiNode = oNode["LocalSpaceRadii"];
            for (size_t i = 0; i < localSpaceRadiiNode.size(); i++) {
                oc.Object.AddLocalSpace(localSpaceRadiiNode[i].as<float>());
            }
            oc.LocalSpaces.clear();
            oc.Object.GetLocalSpaces(oc.LocalSpaces);

            LV_YAML_DESERIALIZE_NODE(oNode, "UIColor",              Vector3,   oc.UIColor);
            LV_YAML_DESERIALIZE_NODE(oNode, "Albedo",               float,     oc.Albedo);
            LV_YAML_DESERIALIZE_NODE(oNode, "ShowMajorMinorAxes",   bool,      oc.ShowMajorMinorAxes);
            LV_YAML_DESERIALIZE_NODE(oNode, "ShowNormal",           bool,      oc.ShowNormal);

            if (!isRootEntity) {
                ((OrbitalScene*)scene)->m_PhysicsToEnttIds.insert({ oc.Object.Id(), entity.m_EnttId });
            }
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
        out << YAML::Key << "Scene" << YAML::Value << std::filesystem::path(filepath).filename().string();

        Scene *pScriptEngineInitialContext = ScriptEngine::GetContext();
        scene->ScriptEngineUseContext();

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
        // Serialise in descending scene hierarchy order,
        // so each entity is deserialised after its parent and can be immediately parented
        SerializeEntity(scene, out, scene->GetRoot());
        auto entities = scene->GetTree(scene->GetRoot());
        for (auto entity : entities) {
            if (!entity) return;
            SerializeEntity(scene, out, entity);
        }
        out << YAML::EndSeq;

        pScriptEngineInitialContext->ScriptEngineUseContext();

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

        Scene *pScriptEngineInitialContext = ScriptEngine::GetContext();
        scene->ScriptEngineUseContext();

        if (auto entitiesNode = data["Entities"]) {
            for (auto entityNode : entitiesNode) {
                DeserializeEntity(scene, entityNode);
            }
        }

        pScriptEngineInitialContext->ScriptEngineUseContext();

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

        LV_YAML_SERIALIZE_NODE(out, "OrbitalScene", std::filesystem::path(filepath).filename().string());

        LV_YAML_SERIALIZE_NODE(out, "LocalSpaceColor",           scene->m_LocalSpaceColor);
        LV_YAML_SERIALIZE_NODE(out, "LocalSpaceThickness",       scene->m_LocalSpaceThickness);
        LV_YAML_SERIALIZE_NODE(out, "LocalSpaceFade",            scene->m_LocalSpaceFade);
        LV_YAML_SERIALIZE_NODE(out, "ShowViewSpace",             scene->m_ShowViewSpace);
        LV_YAML_SERIALIZE_NODE(out, "OrbitThickness",            scene->m_OrbitThickness);
        LV_YAML_SERIALIZE_NODE(out, "OrbitFade",                 scene->m_OrbitFade);
        LV_YAML_SERIALIZE_NODE(out, "OrbitAlpha",                scene->m_OrbitAlpha);
        LV_YAML_SERIALIZE_NODE(out, "OrbitPointRadius",          scene->m_OrbitPointRadius);
        LV_YAML_SERIALIZE_NODE(out, "ShowReferenceAxes",         scene->m_ShowReferenceAxes);
        LV_YAML_SERIALIZE_NODE(out, "ReferenceAxisColor",        scene->m_ReferenceAxisColor);
        LV_YAML_SERIALIZE_NODE(out, "ReferenceAxisLength",       scene->m_ReferenceAxisLength);
        LV_YAML_SERIALIZE_NODE(out, "ReferenceAxisThickness",    scene->m_ReferenceAxisThickness);
        LV_YAML_SERIALIZE_NODE(out, "ReferenceAxisArrowSize",    scene->m_ReferenceAxisArrowSize);
        LV_YAML_SERIALIZE_NODE(out, "PerifocalAxisThickness",    scene->m_PerifocalAxisThickness);
        LV_YAML_SERIALIZE_NODE(out, "PerifocalAxisArrowSize",    scene->m_PerifocalAxisArrowSize);

        LV_YAML_SERIALIZE_NODE(out, "TrackingEntity",           (uint64_t)(scene->m_TrackingEntity));
        LV_YAML_SERIALIZE_NODE(out, "RelativeViewSpace",        scene->m_RelativeViewSpace);

        Scene *pScriptEngineInitialContext = ScriptEngine::GetContext();
        scene->ScriptEngineUseContext();

        scene->PhysicsUseContext();

        LV_YAML_SERIALIZE_NODE(out, "RootScaling",              scene->GetRootScaling());

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
        // Serialise in descending scene hierarchy order,
        // so each entity is deserialised after its parent and can be immediately parented
        SerializeEntity(scene, out, scene->GetRoot());
        auto entities = scene->GetTree(scene->GetRoot());
        for (auto entity : entities)
        {
            if (!entity) return;
            SerializeEntity(scene, out, entity);
        }
        out << YAML::EndSeq;

        pScriptEngineInitialContext->ScriptEngineUseContext();
        // TODO : restore initial physics context

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

        LV_YAML_DESERIALIZE_NODE(data, "LocalSpaceColor",            Vector4,    scene->m_LocalSpaceColor);
        LV_YAML_DESERIALIZE_NODE(data, "LocalSpaceThickness",        float,      scene->m_LocalSpaceThickness);
        LV_YAML_DESERIALIZE_NODE(data, "LocalSpaceFade",             float,      scene->m_LocalSpaceFade);
        LV_YAML_DESERIALIZE_NODE(data, "ShowViewSpace",              bool,       scene->m_ShowViewSpace);
        LV_YAML_DESERIALIZE_NODE(data, "OrbitThickness",             float,      scene->m_OrbitThickness);
        LV_YAML_DESERIALIZE_NODE(data, "OrbitFade",                  float,      scene->m_OrbitFade);
        LV_YAML_DESERIALIZE_NODE(data, "OrbitAlpha",                 float,      scene->m_OrbitAlpha);
        LV_YAML_DESERIALIZE_NODE(data, "OrbitPointRadius",           float,      scene->m_OrbitPointRadius);
        LV_YAML_DESERIALIZE_NODE(data, "ShowReferenceAxes",          bool,       scene->m_ShowReferenceAxes);
        LV_YAML_DESERIALIZE_NODE(data, "ReferenceAxisColor",         Vector4,    scene->m_ReferenceAxisColor);
        LV_YAML_DESERIALIZE_NODE(data, "ReferenceAxisLength",        float,      scene->m_ReferenceAxisLength);
        LV_YAML_DESERIALIZE_NODE(data, "ReferenceAxisThickness",     float,      scene->m_ReferenceAxisThickness);
        LV_YAML_DESERIALIZE_NODE(data, "ReferenceAxisArrowSize",     float,      scene->m_ReferenceAxisArrowSize);
        LV_YAML_DESERIALIZE_NODE(data, "PerifocalAxisThickness",     float,      scene->m_PerifocalAxisThickness);
        LV_YAML_DESERIALIZE_NODE(data, "PerifocalAxisArrowSize",     float,      scene->m_PerifocalAxisArrowSize);

        scene->ScriptEngineUseContext();
        scene->m_PhysicsContext = OrbitalPhysics::Context(); /* reset physics context */
        scene->PhysicsUseContext();

        LV_YAML_DESERIALIZE_NODE_WITH_SETTER(data, "RootScaling", double, scene->SetRootScaling);

        // Entities
        if (auto entitiesNode = data["Entities"]) {
            for (auto entityNode : entitiesNode) {
                DeserializeEntity(scene, entityNode);
            }
        }

        // Editor view space
        UUID trackingEntityId = UUID::Null;
        LV_YAML_DESERIALIZE_NODE(data, "TrackingEntity", uint64_t, trackingEntityId);
        if (trackingEntityId != UUID::Null)
            scene->SetTrackingEntity(scene->GetEntity(trackingEntityId));
        LV_YAML_DESERIALIZE_NODE_WITH_SETTER(data, "RelativeViewSpace", int, scene->SetRelativeViewSpace);

        return true;
    }


    bool SceneSerializer::DeserializeRuntime(OrbitalScene* scene, const std::string& filepath)
    {
        LV_CORE_ASSERT(false, "Not yet implemented!");
        return false;
    }

}
