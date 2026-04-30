#pragma once

#include "AudioBuffer.h"
#include "rack.hpp"
#include "FastLogTable.h"
#include "FastPowTable.h"
#include "MemoryBuffer.hpp"
#include "PatchProcessor.h"

static float parameter_values[40] = {};
static uint32_t button_values = 0;
int errorcode = 0;

#define BLOCKSIZE 32

struct ProgramVector {};
ProgramVector programVector;
extern int errorcode;

#if defined(VCV)
PatchProcessor::PatchProcessor() : patch(NULL), index(0), bufferCount(0), parameterCount(0), name(NULL) {
	for (int i = 0; i < MAX_NUMBER_OF_PARAMETERS; ++i)
		parameters[i] = NULL;
}
PatchProcessor::~PatchProcessor() {}

static PatchProcessor vcvPatchProcessor;
PatchProcessor* getInitialisingPatchProcessor() {
	return &vcvPatchProcessor;
}
#endif

extern "C"{

  void error(int8_t code, const char* reason){
    printf("%s\n", reason);
    errorcode = -1;
    exit(errorcode);
  }
}

extern "C" {
	void vApplicationMallocFailedHook(void) {
		// error(OUT_OF_MEMORY_ERROR_STATUS, "Memory overflow");
	}
}

extern "C" {
	void doSetButton(uint8_t bid, uint16_t value, uint16_t samples) {
		// DEBUG("Set button B%d: %d\n", bid - 4, value);
		if (value)
			button_values |= (1 << bid);
		else
			button_values &= ~(1 << bid);
	}
	void doSetPatchParameter(uint8_t pid, int16_t value) {
		//if (pid < PARAMETER_AA)
			//DEBUG("Set parameter %c: %d\n", 'A' + pid, value);
		//else
			//DEBUG("Set parameter %c%c: %d\n", '@' + (pid / 8), 'A' + (pid % 8), value);
		if (pid < 40)
			parameter_values[pid] = value / 4096.0f;
	}

	void assert_failed(const char* msg, const char* location, int line) {
		DEBUG("Assertion failed: %s, in %s line %d\n", msg, location, line);
		exit(-1);
	}
}

const char hexnumerals[] = "0123456789abcdef";
char* msg_itoa(int val, int base) {
	static char buf[13] = {0};
	int i = 11;
	unsigned int part = abs(val);
	do {
		buf[i--] = hexnumerals[part % base];
		part /= base;
	} while (part && i);
	if (val < 0)
		buf[i--] = '-';
	return &buf[i + 1];
}

char* msg_ftoa(float val, int base) {
	static char buf[16] = {0};
	int i = 14;
	// print 2 decimal points
	unsigned int part = abs((int)((val - int(val)) * 100));
	do {
		buf[i--] = hexnumerals[part % base];
		part /= base;
	} while (i > 12);
	buf[i--] = '.';
	part = abs(int(val));
	do {
		buf[i--] = hexnumerals[part % base];
		part /= base;
	} while (part && i);
	if (val < 0.0f)
		buf[i--] = '-';
	return &buf[i + 1];
}

void debugMessage(char const* msg, int a) {
	DEBUG("%s %d\n", msg, a);
}

void debugMessage(char const* msg, float a) {
	DEBUG("%s %f\n", msg, a);
}

void debugMessage(char const* msg, int a, int b) {
	DEBUG("%s %d %d\n", msg, a, b);
}

void debugMessage(char const* msg, float a, float b) {
	printf("%s %f %f\n", msg, a, b);
}

void debugMessage(char const* msg, int a, int b, int c) {
	printf("%s %d %d %d\n", msg, a, b, c);
}

void debugMessage(char const* msg, float a, float b, float c) {
	DEBUG("%s %f %f %f\n", msg, a, b, c);
}

void debugMessage(char const* msg) {
	DEBUG("%s\n", msg);
}

// AudioBuffer* AudioBuffer::create(int channels, int samples){
//   return new ManagedMemoryBuffer(channels, samples);
// }
// AudioBuffer::~AudioBuffer(){}
// void AudioBuffer::destroy(AudioBuffer* buffer){
//   delete buffer;
// }

const uint16_t Patch::ON = 4095;
const uint16_t Patch::OFF = 0;

Patch::Patch() {}
Patch::~Patch() {}


Resource* Patch::getResource(char const* name) {
	printf("Get resource %s\n", name);
	Resource* resource = Resource::load(name);

	return resource;
}

void Patch::registerParameter(PatchParameterId pid, const char* name) {
	printf("Register parameter %c: %s\n", 'A' + pid, name);
}

float Patch::getElapsedBlockTime() {
	return 0;
}
int Patch::getElapsedCycles() {
	return 0;
}

void Patch::processMidi(MidiMessage msg) {}

void Patch::sendMidi(MidiMessage msg) {
	printf("Sending MIDI [%x:%x:%x:%x]\n", msg.data[0], msg.data[1], msg.data[2], msg.data[3]);
}

float Patch::getSampleRate() {
	return APP->engine->getSampleRate();
}

int Patch::getBlockSize() {
	return BLOCKSIZE;
}

float Patch::getBlockRate() {
	return APP->engine->getSampleRate() / BLOCKSIZE;
}

int Patch::getNumberOfChannels() {
	return 2;
}

float Patch::getParameterValue(PatchParameterId pid) {
	if (pid < 40)
		return parameter_values[pid];
	return 0.0f;
}

void Patch::setParameterValue(PatchParameterId pid, float value) {
	doSetPatchParameter(pid, value * 4095);
}

void Patch::setButton(PatchButtonId bid, uint16_t value, uint16_t samples) {
	doSetButton(bid, value, samples);
}

bool Patch::isButtonPressed(PatchButtonId bid) {
	return button_values & (1 << bid);
}



AudioBuffer* AudioBuffer::create(int channels, int samples){
  return new ManagedMemoryBuffer(channels, samples);
}
AudioBuffer::~AudioBuffer(){}
void AudioBuffer::destroy(AudioBuffer* buffer){
  delete buffer;
}

class StereoSampleBuffer : public AudioBuffer {

public:
	FloatArray left;
	FloatArray right;
	uint16_t size;

	StereoSampleBuffer(int blocksize) {
		left = FloatArray::create(blocksize);
		right = FloatArray::create(blocksize);
		size = blocksize;
	}
	~StereoSampleBuffer() {
		FloatArray::destroy(left);
		FloatArray::destroy(right);
	}

	void clear() {
		left.clear();
		right.clear();
	}
	inline FloatArray getSamples(int channel) {
		return channel == LEFT_CHANNEL ? left : right;
		// return channel == 0 ? FloatArray(left, size) : FloatArray(right, size);
	}
	inline int getChannels() {
		return 2;
	}
	inline size_t getSize() {
		return size;
	}
};