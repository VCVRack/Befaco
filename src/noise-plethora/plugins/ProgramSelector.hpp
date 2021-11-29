#pragma once

#include "Banks.hpp"

#define CLAMP(x, a, b) (x) < (a) ? (a) : (x) > (b) ? (b) : (x);

enum {
	PROG_A = 0,
	PROG_B
};

class Selection {

public:

	Selection(int numPrograms = programsPerBank)
		: value(0)
		, min(0)
		, max(numPrograms - 1)
	{}

	int getValue() {
		return value;
	}

	int setValue(int val, int numValues = -1) {
		if (numValues == -1) {
			numValues = max + 1;
		}
		val = CLAMP(val, min, numValues - 1);
		value = val;
		return value;
	}

	void setMin(int v) {
		min = v;
	}

	void setMax(int v) {
		max = v;
	}

private:

	int value;
	int min;
	int max;

};

class ProgramSelection {

public:

	int getBank() {
		return bank.getValue();
	}
	int getProgram() {
		return program.getValue();
	}
	int setBank(int b) {
		if (getBankForIndex(b).getSize()) { // assumed that only top bank can be empty
			return bank.setValue(b);
		}
		return getBank();
	}
	int setProgram(int p) {
		return program.setValue(p, getBankForIndex(getBank()).getSize());
	}

	const std::string getCurrentProgramName() {
		return getBankForIndex(getBank()).getProgramName(getProgram());
	}

	float getCurrentProgramGain() {
		return getBankForIndex(getBank()).getProgramGain(getProgram());
	}

private:

	Selection bank{numBanks};
	Selection program;

};

class ProgramSelector {

public:

	ProgramSelector()
		: currentProgram(PROG_A)
	{}

	~ProgramSelector() {}

	ProgramSelection& getCurrent() {
		return currentProgram == PROG_A ? progA : progB;
	}
	ProgramSelection& getA() {
		return progA;
	}
	ProgramSelection& getB() {
		return progB;
	}

	ProgramSelection& getSection(unsigned int program) {
		return program ? progB : progA;
	}

	unsigned int getMode() {
		return currentProgram;
	}
	void setMode(unsigned int current) {
		currentProgram = current ? PROG_B : PROG_A;
	}

private:

	ProgramSelection progA;
	ProgramSelection progB;
	unsigned int currentProgram;

};
