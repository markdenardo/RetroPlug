#include "AudioController.h"

void AudioController::setNode(Node* node) {
	_node = node;
	node->getAllocator()->reserveChunks(160 * 144 * 4, 16); // Video buffers

	_processingContext.setNode(node);

	node->on<calls::SwapLuaContext>([&](const AudioLuaContextPtr& ctx, AudioLuaContextPtr& other) {
		std::string componentData;
		if (_lua) {
			componentData = _lua->serializeInstances();
		}
		
		other = _lua;
		ctx->init(&_processingContext, _timeInfo, _sampleRate);

		if (!componentData.empty()) {
			ctx->deserializeInstances(componentData);
		}
		
		_lua = ctx;
	});

	node->on<calls::SwapSystem>([&](const SystemSwapDesc& d, SystemSwapDesc& other) {
		_lua->addInstance(d.idx, d.instance, *d.componentState);
		
		other.instance = _processingContext.swapInstance(d.idx, d.instance);
		other.componentState = d.componentState;
	});

	node->on<calls::DuplicateSystem>([&](const SystemDuplicateDesc& d, SameBoyPlugPtr& other) {
		other = _processingContext.duplicateInstance(d.sourceIdx, d.targetIdx, d.instance);
		_lua->duplicateInstance(d.sourceIdx, d.targetIdx, d.instance);
	});

	node->on<calls::ResetSystem>([&](const ResetSystemDesc& d) {
		_processingContext.resetInstance(d.idx, d.model);
	});

	node->on<calls::TakeSystem>([&](const SystemIndex& idx, SameBoyPlugPtr& other) {
		_lua->removeInstance(idx);
		other = _processingContext.removeInstance(idx);
	});

	node->on<calls::SetActive>([&](const SystemIndex& idx) {
		_lua->setActive(idx);
	});

	node->on<calls::UpdateProjectSettings>([&](const Project::Settings& settings) {
		_processingContext.setSettings(settings);
	});

	node->on<calls::UpdateSystemSettings>([&](const SystemSettings& settings) {
		_processingContext.setSystemSettings(settings.idx, settings.settings);
	});

	node->on<calls::FetchState>([&](const FetchStateRequest& req, FetchStateResponse& state) {
		fetchState(req, state);
	});

	node->on<calls::SetSram>([&](const SetDataRequest& req, DataBufferPtr& ret) {
		SameBoyPlugPtr inst = _processingContext.getInstance(req.idx);
		if (inst) {
			inst->loadBattery(req.buffer->data(), req.buffer->size(), req.reset);
		}
	
		ret = req.buffer;
	});

	node->on<calls::SetRom>([&](const SetDataRequest& req, DataBufferPtr& ret) {
		SameBoyPlugPtr inst = _processingContext.getInstance(req.idx);
		if (inst) {
			inst->setRomData(req.buffer.get());
			if (req.reset) {
				inst->reset(inst->getSettings().model, true);
			}
		}

		ret = req.buffer;
	});

	node->on<calls::PressButtons>([&](const ButtonPressState& presses) {
		SameBoyPlugPtr& instance = _processingContext.getInstance(presses.idx);
		if (instance) { 
			instance->pressButtons(presses.buttons.presses.data(), presses.buttons.pressCount);
		}
	});

	node->on<calls::ContextMenuResult>([&](const int& id) {
		_lua->onMenuResult(id);
	});
}

void AudioController::fetchState(const FetchStateRequest& req, FetchStateResponse& state) {
	for (size_t i = 0; i < MAX_SYSTEMS; ++i) {
		if (_processingContext.getInstance(i)) {
			state.components[i] = _lua->serializeInstance(i);
		}
	}

	_processingContext.fetchState(req, state);
}
