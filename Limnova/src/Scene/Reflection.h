#pragma once

#ifndef LV_REFL_MAX_NUM_FIELDS
    #define LV_REFL_MAX_NUM_FIELDS 128
#endif

namespace Limnova
{
    namespace Reflection
    {

        enum TypeName
        {
            enum_INVALID,
            enum_int8_t,
            enum_int16_t,
            enum_int32_t,
            enum_uint8_t,
            enum_uint16_t,
            enum_uint32_t,
            enum_float,
            enum_double,
            enum_class
        };

        struct Class;
        struct Type
        {
            ::std::string StringName;
            TypeName EnumName;
            size_t Size;
            Class* ClassInfo;
        };

        template<typename T>
        Type* GetType()
        {
            static Type type;
            type.StringName = "INVALID";
            type.EnumName = TypeName::enum_INVALID;
            type.Size = 0;
            return &type;
        }

        #define REFLECTION_DEFINE_TYPE(TYPE)                                    \
        template<>                                                              \
        ::Limnova::Reflection::Type* ::Limnova::Reflection::GetType<TYPE>()     \
        {                                                                       \
            static ::Limnova::Reflection::Type type;                            \
            type.StringName = #TYPE;                                            \
            type.EnumName = ::Limnova::Reflection::TypeName::enum_##TYPE;       \
            type.Size = sizeof(TYPE);                                           \
            return &type;                                                       \
        }                                                                       \

        struct Field
        {
            Type* Type;
            ::std::string Name;
            size_t Offset;
        };

        // MAX_NUMBER_OF_FIELDS is arbitrarily large
        struct Class
        {
            ::std::string Name;
            ::std::array<Field, LV_REFL_MAX_NUM_FIELDS> Fields;
        };

        template<typename T>
        Class* GetClass()
        {
            static Class localClass;
            return &localClass;
        }

        #define LV_BEGIN_DECLARE_CLASS(CLASS)                                       \
        template<>                                                                  \
        ::Limnova::Reflection::Class* ::Limnova::Reflection::GetClass<CLASS>();     \
                                                                                    \
        template<>                                                                  \
        ::Limnova::Reflection::Type* ::Limnova::Reflection::GetType<CLASS>()        \
        {                                                                           \
            static ::Limnova::Reflection::Type type;                                \
            type.StringName = #CLASS;                                               \
            type.EnumName = ::Limnova::Reflection::TypeName::enum_class;            \
            type.Size = sizeof(CLASS);                                              \
            type.ClassInfo = GetClass<CLASS>();                                     \
            return &type;                                                           \
        }                                                                           \
                                                                                    \
        template<>                                                                  \
        ::Limnova::Reflection::Class* ::Limnova::Reflection::GetClass<CLASS>() {    \
            using ClassType = CLASS;                                                \
            static ::Limnova::Reflection::Class localClass;                         \
            localClass.Name = #CLASS;                                               \
            enum { BASE = __COUNTER__ };                                            \

        #define LV_DECLARE_CLASSMEMBER(NAME)                                        \
        enum { NAME##Index = __COUNTER__ - BASE - 1 };                              \
        localClass.Fields[NAME##Index].Type = GetType<decltype(ClassType::NAME)>(); \
        localClass.Fields[NAME##Index].Name = { #NAME };                            \
        localClass.Fields[NAME##Index].Offset = offsetof(ClassType, NAME);          \

        #define LV_END_DECLARE_CLASS    \
            return &localClass;         \
        }                               \

        #define LV_REFLECT(CLASS) \
        friend ::Limnova::Reflection::Class* ::Limnova::Reflection::GetClass<CLASS>();


#ifdef LV_REFLECTION_EXPOSE_USER
        #define LV_DECLARE_CLASS(CLASS)                                         \
        template<>                                                              \
        ::Limnova::Reflection::Class* ::Limnova::Reflection::GetClass<CLASS>(); \
                                                                                \
        template<>                                                              \
        ::Limnova::Reflection::Type* ::Limnova::Reflection::GetType<CLASS>()    \
        {                                                                       \
            static ::Limnova::Reflection::Type type;                            \
            type.StringName = #CLASS;                                           \
            type.EnumName = ::Limnova::Reflection::TypeName::enum_class;        \
            type.Size = sizeof(CLASS);                                          \
            type.ClassInfo = GetClass<CLASS>();                                 \
            return &type;                                                       \
        }                                                                       \
                                                                                \
        template<>                                                              \
        ::Limnova::Reflection::Class* ::Limnova::Reflection::GetClass<CLASS>()  \
        {                                                                       \
            static ::Limnova::Reflection::Class classInfo;                      \
            classInfo.Name = #CLASS;                                            \
            enum { BASE = __COUNTER__ };                                        \
            return &classInfo;                                                  \
        }                                                                       \
        class CLASS                                                             \

        #define LV_DECLARE_MEMBER(TYPE, NAME)                                   \
        TYPE NAME;                                                              \
        {                                                                       \
            using ClassType = decltype(*this);                                  \
            ::Limnova::Reflection::Class classInfo = GetClass<ClassType>();     \
            enum { NAME##Index = __COUNTER__ - BASE - 1 };                      \
            classInfo.Fields[NAME##Index].Type = GetType<TYPE>();               \
            classInfo.Fields[NAME##Index].Name = { #NAME };                     \
            classInfo.Fields[NAME##Index].Offset = offsetof(ClassType, NAME);   \
        }
#endif

    }
}
