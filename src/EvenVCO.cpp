#include "Befaco.hpp"
#include "dsp.hpp"


struct EvenVCO : Module {
	enum ParamIds {
		OCTAVE_PARAM,
		TUNE_PARAM,
		PWM_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH1_INPUT,
		PITCH2_INPUT,
		FM_INPUT,
		SYNC_INPUT,
		PWM_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TRI_OUTPUT,
		SINE_OUTPUT,
		EVEN_OUTPUT,
		SAW_OUTPUT,
		SQUARE_OUTPUT,
		NUM_OUTPUTS
	};

	float phase = 0.0;
	/** The value of the last sync input */
	float sync = 0.0;
	/** The outputs */
	float tri = 0.0;
	/** Whether we are past the pulse width already */
	bool halfPhase = false;

	MinBLEP<16> triSquareMinBLEP;
	MinBLEP<16> triMinBLEP;
	MinBLEP<16> sineMinBLEP;
	MinBLEP<16> doubleSawMinBLEP;
	MinBLEP<16> sawMinBLEP;
	MinBLEP<16> squareMinBLEP;

	RCFilter triFilter;

	EvenVCO();
	void step();
};


EvenVCO::EvenVCO() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);

	triSquareMinBLEP.minblep = minblep_16_32;
	triSquareMinBLEP.oversample = 32;
	triMinBLEP.minblep = minblep_16_32;
	triMinBLEP.oversample = 32;
	sineMinBLEP.minblep = minblep_16_32;
	sineMinBLEP.oversample = 32;
	doubleSawMinBLEP.minblep = minblep_16_32;
	doubleSawMinBLEP.oversample = 32;
	sawMinBLEP.minblep = minblep_16_32;
	sawMinBLEP.oversample = 32;
	squareMinBLEP.minblep = minblep_16_32;
	squareMinBLEP.oversample = 32;
}

void EvenVCO::step() {
	// Compute frequency, pitch is 1V/oct
	float pitch = 1.0 + roundf(params[OCTAVE_PARAM]) + params[TUNE_PARAM] / 12.0;
	pitch += getf(inputs[PITCH1_INPUT]) + getf(inputs[PITCH2_INPUT]);
	pitch += getf(inputs[FM_INPUT]) / 4.0;
	float freq = 261.626 * powf(2.0, pitch);
	freq = clampf(freq, 0.0, 20000.0);

	// Pulse width
	float pw = params[PWM_PARAM] + getf(inputs[PWM_INPUT]) / 5.0;
	const float minPw = 0.05;
	pw = rescalef(clampf(pw, -1.0, 1.0), -1.0, 1.0, minPw, 1.0-minPw);

	// Advance phase
	float deltaPhase = clampf(freq / gSampleRate, 1e-6, 0.5);
	float oldPhase = phase;
	phase += deltaPhase;

	if (oldPhase < 0.5 && phase >= 0.5) {
		float crossing = -(phase - 0.5) / deltaPhase;
		triSquareMinBLEP.jump(crossing, 2.0);
		doubleSawMinBLEP.jump(crossing, -2.0);
	}

	if (!halfPhase && phase >= pw) {
		float crossing  = -(phase - pw) / deltaPhase;
		squareMinBLEP.jump(crossing, 2.0);
		halfPhase = true;
	}

	// Reset phase if at end of cycle
	if (phase >= 1.0) {
		phase -= 1.0;
		float crossing = -phase / deltaPhase;
		triSquareMinBLEP.jump(crossing, -2.0);
		doubleSawMinBLEP.jump(crossing, -2.0);
		squareMinBLEP.jump(crossing, -2.0);
		sawMinBLEP.jump(crossing, -2.0);
		halfPhase = false;
	}

	// Outputs
	float triSquare = (phase < 0.5) ? -1.0 : 1.0;
	triSquare += triSquareMinBLEP.shift();

	// Integrate square for triangle
	tri += 4.0 * triSquare * freq / gSampleRate;
	tri *= (1.0 - 40.0 / gSampleRate);

	float sine = -cosf(2*M_PI * phase);
	float doubleSaw = (phase < 0.5) ? (-1.0 + 4.0*phase) : (-1.0 + 4.0*(phase - 0.5));
	doubleSaw += doubleSawMinBLEP.shift();
	float even = 0.55 * (doubleSaw + 1.27 * sine);
	float saw = -1.0 + 2.0*phase;
	saw += sawMinBLEP.shift();
	float square = (phase < pw) ? -1.0 : 1.0;
	square += squareMinBLEP.shift();

	// Set outputs
	setf(outputs[TRI_OUTPUT], 5.0*tri);
	setf(outputs[SINE_OUTPUT], 5.0*sine);
	setf(outputs[EVEN_OUTPUT], 5.0*even);
	setf(outputs[SAW_OUTPUT], 5.0*saw);
	setf(outputs[SQUARE_OUTPUT], 5.0*square);
}


EvenVCOWidget::EvenVCOWidget() {
	EvenVCO *module = new EvenVCO();
	setModule(module);
	box.size = Vec(15*8, 380);

	{
		Panel *panel = new DarkPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/Befaco/res/EvenVCO.png");
		addChild(panel);
	}

	addChild(createScrew<ScrewBlack>(Vec(15, 0)));
	addChild(createScrew<ScrewBlack>(Vec(15, 365)));
	addChild(createScrew<ScrewBlack>(Vec(15*6, 0)));
	addChild(createScrew<ScrewBlack>(Vec(15*6, 365)));

	addParam(createParam<BefacoBigSnapKnob>(Vec(24-4+2, 35-4+1), module, EvenVCO::OCTAVE_PARAM, -5.0, 4.0, 0.0));
	addParam(createParam<BefacoTinyKnob>(Vec(72, 131), module, EvenVCO::TUNE_PARAM, -7.0, 7.0, 0.0));
	addParam(createParam<Davies1900hRedKnob>(Vec(16, 230), module, EvenVCO::PWM_PARAM, -1.0, 1.0, 0.0));

	addInput(createInput<PJ3410Port>(Vec(13-7-1, 124-7), module, EvenVCO::PITCH1_INPUT));
	addInput(createInput<PJ3410Port>(Vec(22-7, 162-7-1), module, EvenVCO::PITCH2_INPUT));
	addInput(createInput<PJ3410Port>(Vec(51-7, 188-7-1), module, EvenVCO::FM_INPUT));
	addInput(createInput<PJ3410Port>(Vec(88-7, 193-7), module, EvenVCO::SYNC_INPUT));

	addInput(createInput<PJ3410Port>(Vec(76-7, 240-7), module, EvenVCO::PWM_INPUT));

	addOutput(createOutput<PJ3410Port>(Vec(12-7, 285-7), module, EvenVCO::TRI_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(88-7+1, 285-7), module, EvenVCO::SINE_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(50-7+1, 308-7), module, EvenVCO::EVEN_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(12-7, 329-7), module, EvenVCO::SAW_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(88-7+1, 329-7), module, EvenVCO::SQUARE_OUTPUT));
}
