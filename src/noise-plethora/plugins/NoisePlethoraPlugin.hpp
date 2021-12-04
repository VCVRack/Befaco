#pragma once

#include <rack.hpp>
#include <memory>
#include <string> // string might not be allowed
#include <functional>
#include <map>

#include "../teensy/TeensyAudioReplacements.hpp"


class NoisePlethoraPlugin {

public:
	NoisePlethoraPlugin() {	}
	virtual ~NoisePlethoraPlugin() {}

	NoisePlethoraPlugin(const NoisePlethoraPlugin&) = delete;
	NoisePlethoraPlugin& operator=(const NoisePlethoraPlugin&) = delete;

	virtual void init() {};
	// equivelent to arduino void loop()
	virtual void process(float k1, float k2) {};

	// called once-per sample, will consume a buffer of length AUDIO_BLOCK_SAMPLES (128)
	// then request that the buffer be refilled, returns values in range [-1, 1]
	float processGraph() {

		if (blockBuffer.empty()) {
			processGraphAsBlock(blockBuffer);
		}

		return int16_to_float_1v(blockBuffer.shift());
	}

	virtual AudioStream& getStream() = 0;
	virtual unsigned char getPort() = 0;

protected:

	// subclass should process the audio graph and fill the supplied buffer
	virtual void processGraphAsBlock(TeensyBuffer& blockBuffer) = 0;

	TeensyBuffer blockBuffer;
};


class MyFactory {
public:
	static MyFactory* Instance() {
		static MyFactory factory;
		return &factory;
	}

	std::shared_ptr<NoisePlethoraPlugin> Create(std::string name) {
		NoisePlethoraPlugin* instance = nullptr;

		// find name in the registry and call factory method.
		auto it = factoryFunctionRegistry.find(name);
		if (it != factoryFunctionRegistry.end()) {
			instance = it->second();
		}

		// wrap instance in a shared ptr and return
		if (instance != nullptr) {
			return std::shared_ptr<NoisePlethoraPlugin>(instance);
		}
		else {
			return nullptr;
		}
	}

	void RegisterFactoryFunction(std::string name, std::function<NoisePlethoraPlugin*(void)> classFactoryFunction) {
		// register the class factory function
		factoryFunctionRegistry[name] = classFactoryFunction;
	}

	std::map<std::string, std::function<NoisePlethoraPlugin*(void)>> factoryFunctionRegistry;
};

template<class T> class Registrar {
public:
	Registrar(std::string className) {
		// register the class factory function
		MyFactory::Instance()->RegisterFactoryFunction(className,
		    [](void) -> NoisePlethoraPlugin * { return new T();});
	}
};

#define REGISTER_PLUGIN(NAME) static Registrar<NAME> NAME ##_reg(#NAME)