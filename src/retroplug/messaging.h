#pragma once

#include "micromsg/request.h"
#include "micromsg/node.h"
#include "model/Project.h"
#include "model/ButtonStream.h"

class SameBoyPlug;
using SameBoyPlugPtr = std::shared_ptr<SameBoyPlug>;

class AudioLuaContext;
using AudioLuaContextPtr = std::shared_ptr<AudioLuaContext>;

enum class NodeTypes {
	Ui,
	Audio,

	COUNT
};

struct LoadRomDesc {
	// romData
	bool reset = true;
};

struct SystemSwapDesc {
	SystemIndex idx;
	SameBoyPlugPtr instance;
	std::shared_ptr<std::string> componentState;
};

struct SystemDuplicateDesc {
	SystemIndex sourceIdx;
	SystemIndex targetIdx;
	SameBoyPlugPtr instance;
};

struct VideoBuffer {
	Dimension2 dimensions;
	micromsg::UniquePtr<char> data;
};

struct VideoStream {
	VideoBuffer buffers[MAX_SYSTEMS];
};

struct AudioBuffer {
	size_t frameCount = 0;
	size_t channelCount = 0;
	FloatDataBufferPtr data;
};

enum class ResourceType {
	None = 0,

	Rom = 1<<0,
	Sram = 1<<1,
	State = 1<<2,
	Components = 1<<3,

	AllExceptRom = Sram | State | Components,
	All = Rom | Sram | State | Components
};

struct FetchStateRequest {
	std::array<ResourceType, MAX_SYSTEMS> systems = { ResourceType::None };
	DataBufferPtr srams[MAX_SYSTEMS];
	DataBufferPtr states[MAX_SYSTEMS];
};

struct FetchStateResponse {
	std::array<DataBufferPtr, MAX_SYSTEMS> srams;
	std::array<DataBufferPtr, MAX_SYSTEMS> states;
	std::array<std::string, MAX_SYSTEMS> components;
};

struct ResetSystemDesc {
	SystemIndex idx;
	GameboyModel model;
};

struct FetchSramRequest {
	SystemIndex idx;
	DataBufferPtr buffer;
};

struct SetDataRequest {
	SystemIndex idx;
	DataBufferPtr buffer;
	bool reset;
};

struct ButtonPressState {
	SystemIndex idx;
	ButtonStream<32> buttons;
};

struct SystemSettings {
	SystemIndex idx;
	SameBoySettings settings;
};

#define DefinePush(name, arg) class name : public micromsg::Push<arg> {};
#define DefineRequest(name, arg, ret) class name : public micromsg::Request<arg, ret> {};

namespace calls {
	DefinePush(LoadRom, LoadRomDesc);
	DefinePush(TransmitVideo, VideoStream);
	DefinePush(UpdateProjectSettings, Project::Settings);
	DefinePush(UpdateSystemSettings, SystemSettings);
	DefinePush(PressButtons, ButtonPressState);
	DefinePush(ContextMenuResult, int);
	DefinePush(SetActive, SystemIndex);
	DefinePush(ResetSystem, ResetSystemDesc);
	DefinePush(EnableRendering, bool);

	DefineRequest(SwapLuaContext, AudioLuaContextPtr, AudioLuaContextPtr);
	DefineRequest(SwapSystem, SystemSwapDesc, SystemSwapDesc);
	DefineRequest(SetRom, SetDataRequest, DataBufferPtr);
	DefineRequest(SetSram, SetDataRequest, DataBufferPtr);
	DefineRequest(SetState, SetDataRequest, DataBufferPtr);
	DefineRequest(DuplicateSystem, SystemDuplicateDesc, SameBoyPlugPtr);
	DefineRequest(TakeSystem, SystemIndex, SameBoyPlugPtr);
	DefineRequest(FetchState, FetchStateRequest, FetchStateResponse);

	DefineRequest(FetchSram, FetchSramRequest, DataBufferPtr);
}

using Node = micromsg::Node<NodeTypes>;