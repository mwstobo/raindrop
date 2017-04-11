#include "pch.h"

#include "GameState.h"

#include "Texture.h"
#include "ImageLoader.h"
#include "Sprite.h"
#include "LuaManager.h"
#include "SceneEnvironment.h"
#include "Configuration.h"
#include "LuaBridge.h"

#include "Font.h"
#include "TruetypeFont.h"
#include "BitmapFont.h"
#include "Line.h"

#include "GraphicalString.h"
#include "Logging.h"

#include "Shader.h"

namespace LuaAnimFuncs
{
    const char * SpriteMetatable = "Sys.Sprite";

    int GetSkinConfigF(lua_State *L)
    {
        std::string Key = luaL_checkstring(L, 1);
        std::string Namespace = luaL_checkstring(L, 2);

        lua_pushnumber(L, Configuration::GetSkinConfigf(Key, Namespace));
        return 1;
    }

    int GetSkinConfigS(lua_State *L)
    {
        std::string Key = luaL_checkstring(L, 1);
        std::string Namespace = luaL_checkstring(L, 2);

        lua_pushstring(L, Configuration::GetSkinConfigs(Key, Namespace).c_str());
        return 1;
    }

    int Require(lua_State *L)
    {
        LuaManager *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
        std::string Request = luaL_checkstring(L, 1);
        auto File = GameState::GetInstance().GetSkinScriptFile(Request.c_str(), GameState::GetInstance().GetSkin());
        Lua->Require(File);

		// return whatever lua->require left on the stack.
        return 1;
    }

    int FallbackRequire(lua_State *L)
    {
        LuaManager *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
        std::string file = luaL_checkstring(L, 1);
        std::string skin = GameState::GetInstance().GetFirstFallbackSkin();
        if (lua_isstring(L, 2) && lua_tostring(L, 2))
            skin = luaL_checkstring(L, 2);

        if (skin == GameState::GetInstance().GetSkin())
        {
            lua_pushboolean(L, 0);
            return 1;
        }

        Lua->Require(GameState::GetInstance().GetSkinScriptFile(file.c_str(), skin));
        return 1;
    }

    int GetSkinDirectory(lua_State *L)
    {
        lua_pushstring(L, GameState::GetInstance().GetSkinPrefix().c_str());
        return 1;
    }

    int GetSkinFile(lua_State *L)
    {
        auto Out = GameState::GetInstance().GetSkinFile(std::string(luaL_checkstring(L, 1)), GameState::GetInstance().GetSkin());
        lua_pushstring(L, Utility::ToU8(Out.wstring()).c_str());
        return 1;
    }

    int GetFallbackFile(lua_State *L)
    {
        auto Out = GameState::GetInstance().GetFallbackSkinFile(std::string(luaL_checkstring(L, 1)));
        lua_pushstring(L, Utility::ToU8(Out.wstring()).c_str());
        return 1;
    }
}

// Wrapper functions
void SetImage(Sprite *O, std::string dir)
{
    O->SetImage(GameState::GetInstance().GetSkinImage(dir));
    if (O->GetImage() == nullptr)
        Log::Printf("File %s could not be loaded.\n", dir.c_str());
}

std::string GetImage(const Sprite *O)
{
    return O->GetImageFilename();
}

void AddPosition(Sprite *O, float px, float py)
{
    O->AddPosition(px, py);
}

void SetPosition(Sprite *O, float px, float py)
{
    O->SetPosition(px, py);
}

void LoadBmFont(BitmapFont* B, std::string Fn, float CellWidth, float CellHeight, float CharWidth, float CharHeight, int startChar)
{
    Vec2 Size(CharWidth, CharHeight);
    Vec2 CellSize(CellWidth, CellHeight);
    B->LoadSkinFontImage(Fn.c_str(), Size, CellSize, Size, startChar);
}

// We need these to be able to work with gcc.
// Adding these directly does not work. Inheriting them from Transformation does not work. We're left only with this.
struct O2DProxy
{
    static uint32_t getZ(Sprite const* obj)
    {
        return obj->GetZ();
    }

    static float getScaleX(Sprite const* obj)
    {
        return obj->GetScaleX();
    }

    static float getScaleY(Sprite const* obj)
    {
        return obj->GetScaleY();
    }

    static float getWidth(Sprite const* obj)
    {
        return obj->GetWidth();
    }

    static float getHeight(Sprite const* obj)
    {
        return obj->GetHeight();
    }

    static float getX(Sprite const* obj)
    {
        return obj->GetPositionX();
    }

    static float getY(Sprite const* obj)
    {
        return obj->GetPositionY();
    }

    static float getRotation(Sprite const* obj)
    {
        return obj->GetRotation();
    }

    static Transformation getChainTransformation(Sprite const* obj)
    {
        return Transformation();
    }

    static void setZ(Sprite *obj, uint32_t nZ)
    {
        obj->SetZ(nZ);
    }

    static void setHeight(Sprite *obj, float param)
    {
        obj->SetHeight(param);
    }

    static void setWidth(Sprite *obj, float param)
    {
        obj->SetWidth(param);
    }

    static void setScaleY(Sprite *obj, float param)
    {
        obj->SetScaleY(param);
    }

    static void setScaleX(Sprite *obj, float param)
    {
        obj->SetScaleX(param);
    }

    static void setRotation(Sprite *obj, float param)
    {
        obj->SetRotation(param);
    }

    static void setX(Sprite *obj, float param)
    {
        obj->SetPositionX(param);
    }

    static void setY(Sprite *obj, float param)
    {
        obj->SetPositionY(param);
    }

    static void setChainTransformation(Sprite *obj, Transformation* param)
    {
        obj->ChainTransformation(param);
    }

    static void setScale(Sprite *obj, float param)
    {
        obj->SetScale(param);
    }

    static void AddRotation(Sprite* obj, float param)
    {
        obj->AddRotation(param);
    }
};

class LShader : public Renderer::Shader {
public:
	int Send(lua_State *L) {
		int n = lua_gettop(L);
		std::string sendto = luaL_checkstring(L, 2);

		Bind();
		int uniform = Shader::GetUniform(sendto);

		switch (n) {
		case 3:
			Shader::SetUniform(uniform, (float)luaL_checknumber(L, 3));
			break;
		case 4:
			Shader::SetUniform(uniform, Vec2((float)luaL_checknumber(L, 3), 
										     (float)luaL_checknumber(L, 4)));
			break;
		case 5:
			Shader::SetUniform(uniform, Vec3((float)luaL_checknumber(L, 3), 
											 (float)luaL_checknumber(L, 4), 
											 (float)luaL_checknumber(L, 5)));
			break;
		case 6:
			Shader::SetUniform(uniform, (float)luaL_checknumber(L, 3), 
										(float)luaL_checknumber(L, 4), 
										(float)luaL_checknumber(L, 5),
										(float)luaL_checknumber(L, 6));
			break;
		default:
			return luaL_error(L, "shader send has wrong argument n (%d) range is 3 to 6", n + 1);
		};

		return 0;
	}
};

void DefineSpriteInterface(LuaManager* anim_lua)
{
    anim_lua->AppendPath("./?;./?.lua");
    anim_lua->AppendPath(GameState::GetInstance().GetScriptsDirectory() + "?");
    anim_lua->AppendPath(GameState::GetInstance().GetScriptsDirectory() + "?.lua");

    anim_lua->AppendPath(GameState::GetInstance().GetSkinPrefix() + "?");
    anim_lua->AppendPath(GameState::GetInstance().GetSkinPrefix() + "?.lua");

    // anim_lua->AppendPath(GameState::GetFallbackSkinPrefix());
    anim_lua->Register(LuaAnimFuncs::Require, "skin_require");
    anim_lua->Register(LuaAnimFuncs::FallbackRequire, "fallback_require");

    // anim_lua->NewMetatable(LuaAnimFuncs::SpriteMetatable);
    anim_lua->Register(LuaAnimFuncs::GetSkinConfigF, "GetConfigF");
    anim_lua->Register(LuaAnimFuncs::GetSkinConfigS, "GetConfigS");
    anim_lua->Register(LuaAnimFuncs::GetSkinDirectory, "GetSkinDirectory");
    anim_lua->Register(LuaAnimFuncs::GetSkinFile, "GetSkinFile");

    anim_lua->SetGlobal("ScreenHeight", ScreenHeight);
    anim_lua->SetGlobal("ScreenWidth", ScreenWidth);

    // Animation constants
    anim_lua->SetGlobal("EaseNone", Animation::EaseLinear);
    anim_lua->SetGlobal("EaseIn", Animation::EaseIn);
    anim_lua->SetGlobal("EaseOut", Animation::EaseOut);

    anim_lua->SetGlobal("BlendAdd", (int)BLEND_ADD);
    anim_lua->SetGlobal("BlendAlpha", (int)BLEND_ALPHA);

#define f(x) addFunction(#x, &Sprite::x)
#define p(x) addProperty(#x, &Sprite::Get##x, &Sprite::Set##x)
#define v(x) addData(#x, &Sprite::x)
#define q(x) addProperty(#x, &O2DProxy::get##x, &O2DProxy::set##x)

    luabridge::getGlobalNamespace(anim_lua->GetState())
        .beginClass <Transformation>("Transformation")
        .addConstructor<void(*) ()>()
        .addProperty("Z", &Transformation::GetZ, &Transformation::SetZ)
        .addProperty("Layer", &Transformation::GetZ, &Transformation::SetZ)
        .addProperty("Rotation", &Transformation::GetRotation, &Transformation::SetRotation)
        .addProperty("Width", &Transformation::GetWidth, &Transformation::SetWidth)
        .addProperty("Height", &Transformation::GetHeight, &Transformation::SetHeight)
        .addProperty("ScaleX", &Transformation::GetScaleX, &Transformation::SetScaleX)
        .addProperty("ScaleY", &Transformation::GetScaleY, &Transformation::SetScaleY)
        .addProperty("X", &Transformation::GetPositionX, &Transformation::SetPositionX)
        .addProperty("Y", &Transformation::GetPositionY, &Transformation::SetPositionY)
        .addFunction("SetChainTransformation", &Transformation::ChainTransformation)
        .endClass();

	luabridge::getGlobalNamespace(anim_lua->GetState())
		.beginClass<Renderer::Shader>("__shader_internal")
		.endClass()
		.deriveClass<LShader, Renderer::Shader>("Shader")
		.addConstructor<void(*) ()>()
		.addFunction("Compile", &LShader::Compile)
		.addCFunction("Send", &LShader::Send)
		.endClass();

    luabridge::getGlobalNamespace(anim_lua->GetState())
        .deriveClass<Sprite, Transformation>("Object2D")
        .addConstructor<void(*) ()>()
        .v(Centered)
        .v(Lighten)
        .v(LightenFactor)
        .v(AffectedByLightning)
        .v(ColorInvert)
        .v(Alpha)
        .v(Red)
        .v(Blue)
        .v(Green)
        .p(BlendMode)
        .f(SetCropByPixels)
		.addProperty("Shader", &Sprite::GetShader, &Sprite::SetShader)
        .addFunction<void (Sprite::*)(float)>("AddRotation", &Sprite::AddRotation)
        .addFunction<void (Sprite::*)(float)>("SetScale", &Sprite::SetScale)
        .q(Z)
        .addProperty("Layer", &O2DProxy::getZ, &O2DProxy::setZ)
        .q(ScaleX)
        .q(ScaleY)
        .q(Rotation)
        .q(Width)
        .q(Height)
        .q(X)
        .q(Y)
        .q(ChainTransformation)
        .addProperty("Texture", GetImage, SetImage) // Special for setting image.
        .endClass();
}

// New lua interface.
void CreateNewLuaAnimInterface(LuaManager *AnimLua)
{
    DefineSpriteInterface(AnimLua);

    /*
    luabridge::getGlobalNamespace(AnimLua->GetState())
        .beginClass <Line>("Line2D")
        .addConstructor<void(*) ()>()
        .addFunction("SetColor", &Line::SetColor)
        .addFunction("SetLocation", &Line::SetLocation)
        .endClass();
        */

    luabridge::getGlobalNamespace(AnimLua->GetState())
        .beginClass <SceneEnvironment>("GraphObjMan")
        .addFunction("AddAnimation", &SceneEnvironment::AddLuaAnimation)
        .addFunction("AddTarget", &SceneEnvironment::AddTarget)
        .addFunction("Sort", &SceneEnvironment::Sort)
        .addFunction("StopAnimation", &SceneEnvironment::StopAnimationsForTarget)
        .addFunction("SetUILayer", &SceneEnvironment::SetUILayer)
        .addFunction("RunUIScript", &SceneEnvironment::RunUIScript)
        .addFunction("CreateObject", &SceneEnvironment::CreateObject)
        .endClass();

    luabridge::push(AnimLua->GetState(), GetObjectFromState<SceneEnvironment>(AnimLua->GetState(), "GOMAN"));
    lua_setglobal(AnimLua->GetState(), "Engine");
#undef f
#undef p
#undef v

    // These are mostly defined so we can pass them around and construct them.
    luabridge::getGlobalNamespace(AnimLua->GetState())
        .beginNamespace("Fonts")
        .beginClass<Font>("Font")
        .addFunction("SetColor", &Font::SetColor)
        .addFunction("SetAlpha", &Font::SetAlpha)
        .addFunction("GetLength", &Font::GetHorizontalLength)
        .endClass()
        .deriveClass <TruetypeFont, Font>("TruetypeFont")
        .addConstructor <void(*) (std::string, float)>()
        .endClass()
        .deriveClass <BitmapFont, Font>("BitmapFont")
        .addConstructor<void(*)()>()
        .endClass()
        .addFunction("LoadBitmapFont", LoadBmFont)
        .endNamespace();

    luabridge::getGlobalNamespace(AnimLua->GetState())
        .deriveClass<GraphicalString, Sprite>("StringObject2D")
        .addConstructor <void(*) ()>()
        .addProperty("Font", &GraphicalString::GetFont, &GraphicalString::SetFont)
        .addProperty("Text", &GraphicalString::GetText, &GraphicalString::SetText)
        .endClass();
}

// Old, stateful lua interface.
void CreateLuaInterface(LuaManager *AnimLua)
{
    CreateNewLuaAnimInterface(AnimLua);
}