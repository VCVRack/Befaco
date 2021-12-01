#pragma once

#include <rack.hpp>

#include "PluginFactory.hpp"
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

	virtual AudioStream& getStream();
	virtual unsigned char getPort();

protected:

	// subclass should process the audio graph and fill the supplied buffer
	virtual void processGraphAsBlock(TeensyBuffer& blockBuffer);

	TeensyBuffer blockBuffer;
};

#define REGISTER_PLUGIN(NAME) \
	static Registrar<NAME> NAME ##_reg(#NAME)

