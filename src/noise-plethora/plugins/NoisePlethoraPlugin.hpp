#pragma once

#include <rack.hpp>

#include "PluginFactory.hpp"
#include "../teensy/TeensyAudioReplacements.hpp"


class NoisePlethoraPlugin {

public:
	NoisePlethoraPlugin() {}
	virtual ~NoisePlethoraPlugin() {}

	NoisePlethoraPlugin(const NoisePlethoraPlugin&) = delete;
	NoisePlethoraPlugin& operator=(const NoisePlethoraPlugin&) = delete;

	virtual void init() {};
	// equivelent to arduino void loop()
	virtual void process(float k1, float k2) {};
	// process the audio graph
	virtual float processGraph(float sampleTime) { return 0.f; };
	virtual void onSampleRateChange(float sampleTime) { };

	float getAlternativeOutput() { return altOutput; }


	virtual AudioStream& getStream();
	virtual unsigned char getPort();

protected:
	// just for DEBUG, remove!
	float altOutput = 0.f;
};

#define REGISTER_PLUGIN(NAME) \
    static Registrar<NAME> NAME ##_reg(#NAME)

