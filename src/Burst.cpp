#include "plugin.hpp"


// Ported from https://github.com/Befaco/burst

// Arduino header library

typedef uint8_t byte;
#define HIGH 0x1
#define LOW 0x0

#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#define random(max) (random::u32() % (max))


////// ANALOG INS

#define CV_DIVISIONS    0
#define CV_PROBABILITY  1
#define CV_REPETITIONS  2
#define CV_DISTRIBUTION 3

////// DIGITAL INS

#define CYCLE_SWITCH    0
#define CYCLE_STATE     1

#define PING_STATE      2
#define ENCODER_BUTTON  3
#define ENCODER_1       5
#define ENCODER_2       4
#define TRIGGER_STATE   8
#define TRIGGER_BUTTON  11

////// DIGITAL OUTS

#define TEMPO_STATE     0
#define OUT_LED         1
#define OUT_STATE       7
#define EOC_STATE       9

#define LED_PIN_0 12
#define LED_PIN_1 13
#define LED_PIN_2 10
#define LED_PIN_3 6
static const int ledPin[4] = {LED_PIN_0, LED_PIN_1, LED_PIN_2, LED_PIN_3};

#define MAX_REPETITIONS 32                  /// max number of repetitions

#define TAP_TEMPO_AVG_COUNT (2)
#define TRIGGER_LENGTH (10) // 10ms is a good length to be picked up by other modules
#define ANALOG_MIN 20 // 0 + 20
#define ANALOG_MAX 1003 // 1023 - 20


struct Burst : Module {
	enum ParamIds {
		CYCLE_PARAM,
		QUANTITY_PARAM,
		TRIGGER_PARAM,
		QUANTITY_CV_PARAM,
		DISTRIBUTION_PARAM,
		TIME_PARAM,
		PROBABILITY_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		QUANTITY_INPUT,
		DISTRIBUTION_INPUT,
		PING_INPUT,
		TIME_INPUT,
		PROBABILITY_INPUT,
		TRIGGER_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TEMPO_OUTPUT,
		EOC_OUTPUT,
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(QUANTITY_LIGHTS, 16),
		TEMPO_LIGHT,
		EOC_LIGHT,
		OUT_LIGHT,
		NUM_LIGHTS
	};

	float millisPhase = 0.f;
	unsigned long millis = 0;

	byte eeprom[16] = {};
	byte ledState = 0;
	int loopDivider = 0;

	float quantityValueLast = 0.f;

	////////////////////
	// BURST.ino variables
	////////////////////

	int16_t encoderValue = 0;
	int16_t lastEncoderValue = 0;

	// tempo and counters
	unsigned long masterClock = 0;             /// the master clock is the result of averaged time of ping input or encoder button
	unsigned long masterClock_Temp = 0;        /// we use the temp variables to avoid the parameters change during a burst execution
	unsigned long clockDivided = 0;            /// the result of div/mult the master clock depending on the div/mult potentiometer/input
	unsigned long clockDivided_Temp = 0;            /// we use the temp variables to avoid the parameters change during a burst execution

	float timePortions = 0;                      /// the linear portions of time depending on clock divided and number of repetitions in the burst. if the distribution is linear this will give us the duration of every repetition.
	/// if it is not linear it will be used to calculate the distribution
	float timePortions_Temp = 0;                 /// we use the temp variables to avoid the parameters change during a burst execution

	unsigned long firstBurstTime = 0;         /// the moment when the burst start
	unsigned long burstTimeStart = 0;         /// the moment when the burst start
	unsigned long burstTimeAccu = 0;          /// the accumulation of all repetitions times since the burst have started

	unsigned long repetitionRaiseStart = 0;       /// the time when the previous repetition pulse have raised

	byte repetitionCounter = 0;            /// the current repetition number since the burst has started

	unsigned long elapsedTimeSincePrevRepetition = 0;               /// the difference between previous repetition time and current repetition time. the difference between one repetition and the next one
	unsigned long elapsedTimeSincePrevRepetitionNew = 0;           /// the position of the new repetition inside the time window
	unsigned long elapsedTimeSincePrevRepetitionOld = 0;           /// the position of the previous repetition inside the time window

	unsigned long ledQuantityTime = 0;        /// time counter used to turn off the led when we are chosing the number of repetitions with the encoder
	unsigned long ledParameterTime = 0;        /// time counter used to turn off the led when we are chosing the number of repetitions with the encoder

	bool inEoc = false;
	bool wantsEoc = false;
	unsigned long eocCounter = 0;              /// a counter to turn off the eoc led and the eoc output

	//    divisions
	int divisions;                            //// value of the time division or the time multiplier
	int divisions_Temp = 0;
	int divisionCounter = 0;

	unsigned long triggerButtonPressedTime = 0;

	////// Repetitions

	byte repetitions = 1;                       /// number of repetitions
	byte repetitionsOld = 1;
	byte repetitions_Temp = 1;                  /// temporal value that adds the number of repetitions in the encoder and the number number added by the cv quantity input
	byte repetitionsEncoder = 0;
	byte repetitionsEncoder_Temp = 0;

	///// Random
	int randomPot = 0;
	int randomPot_Temp = 0;

	///// Distribution
	enum {
		DISTRIBUTION_SIGN_NEGATIVE = 0,
		DISTRIBUTION_SIGN_POSITIVE = 1,
		DISTRIBUTION_SIGN_ZERO = 2
	};

	float distribution = 0;
	byte distributionSign = DISTRIBUTION_SIGN_POSITIVE;
	float distributionIndexArray[9];       /// used to calculate the position of the repetitions
	int curve = 5;                         /// the curved we apply to the  std::pow function to calculate the distribution of repetitions
	float distribution_Temp = 0;
	byte distributionSign_Temp = DISTRIBUTION_SIGN_POSITIVE;

	//// Trigger
	uint8_t triggerButtonState = LOW;           /// the trigger button
	bool triggered = false;                        /// the result of both trigger button and trigger input
	bool triggerFirstPressed = 0;

	//// Cycle
	bool cycleSwitchState = 0;             /// the cycle switch
	bool cycleInState = 0;                 /// the cycle input
	bool cycle = false;                    /// the result of both cycle switch and cycle input
	bool resetPhase = false;

	/// ping
	byte pingInState = 0;

	/// output
	bool outputState = LOW;

	bool recycle = false;     // recycle = start a new burst within a set of bursts (mult)
	bool resync = false;      // resync = start a new burst, but ensure that we're correctly phased

	//// encoder button and tap tempo
	byte encoderButtonState = 0;
	unsigned long tempoTic = 0;       // the PREVIOUS tempo tick
	unsigned long tempoTic_Temp = 0;  // the NEXT tempo tick
	unsigned long tempoTimer = 0;
	unsigned long tempoTimer_Temp = 0;

	byte disableFirstClock = 0; // backward since we can't assume that there's anything saved on the EEPROM_	byte probabilityAffectsEOC = 0;
	byte probabilityAffectsEOC = 0;
	byte silentBurst = false;

	enum {
		RETRIGGER = 0,
		NO_RETRIGGER = 1
	};
	byte retriggerMode = RETRIGGER;

	// Bounce bounce;

	bool wantsMoreBursts = HIGH;

	////////////////////
	// functions.ino variables
	////////////////////

	int trigLen = TRIGGER_LENGTH;
	unsigned long encoderLastTime = 0;
	unsigned long encoderTaps[TAP_TEMPO_AVG_COUNT] = {0, 0};
	unsigned long encoderDuration = 0;
	byte encoderTapsCurrent = 0;
	byte encoderTapsTotal = 0; // 2 means use the duration between encoderTaps[0] and encoderTaps[1]; 3 means average of [0-2]

	unsigned long pingLastTime = 0;
	unsigned long pingDuration = 0;

	/// flags
	bool burstStarted = LOW; // if the burst is active or not

	int calibratedRepetitions = 511;
	int calibratedDistribution = 511;
	int calibratedDivisions = 511;
	int calibratedProbability = 511;

	int readDivision_divisionsPotPrev = 0;
	int calibratedCVZero = 719;
	int readDistribution_distributionPotPrev = 0;
	bool tempoInitialized = false;
	unsigned long tempoStart = 0;

	////////////////////
	// methods
	////////////////////

	Burst() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Burst::CYCLE_PARAM, 0.0, 1.0, 0.0, "");
		configParam(Burst::QUANTITY_PARAM, -INFINITY, INFINITY, 0.0, "");
		configParam(Burst::TRIGGER_PARAM, 0.0, 1.0, 0.0, "");
		configParam(Burst::QUANTITY_CV_PARAM, 0.0, 1.0, 0.5, "");
		configParam(Burst::DISTRIBUTION_PARAM, 0.0, 1.0, 0.5, "");
		configParam(Burst::TIME_PARAM, 0.0, 1.0, 0.5, "");
		configParam(Burst::PROBABILITY_PARAM, 0.0, 1.0, 0.0, "");

		updateLeds();
		setup();
	}

	void process(const ProcessArgs &args) override {
		// millis timer
		millisPhase += 1000.f * args.sampleTime;
		if (millisPhase >= 1.f) {
			millisPhase -= 1.f;
			millis++;
		}

		if (++loopDivider >= 16) {
			loopDivider = 0;
			loop();
		}
	}

	////////////////////
	// BURST.ino functions
	////////////////////

	void setup() {
		createDistributionIndex();

		masterClock = (EEPROM_read(0) & 0xff) + (((long)EEPROM_read(1) << 8) & 0xff00) +
		              (((long)EEPROM_read(2) << 16) & 0xff0000) +
		              (((long)EEPROM_read(3) << 24) & 0xff000000);
		if (masterClock < 1) masterClock = 120;
		masterClock_Temp = masterClock;

		repetitions = EEPROM_read(4);
		repetitions = constrain(repetitions, 1, MAX_REPETITIONS);

		disableFirstClock = EEPROM_read(5);

		repetitions_Temp = repetitions;
		repetitionsOld = repetitions;
		repetitionsEncoder = repetitions;
		repetitionsEncoder_Temp = repetitions;

		retriggerMode = EEPROM_read(14);
		probabilityAffectsEOC = EEPROM_read(15);

		if (!checkCalibrationMode()) {
			doLightShow();
		}

		readCycle();
		if (cycle == HIGH) {
			triggered = triggerFirstPressed = HIGH;
		}

		readDivision(0);
		calcTimePortions();
	}

	void loop() {
		unsigned long currentTime = millis;
		unsigned long nextSyncPoint = firstBurstTime + ((divisions > 0) ? clockDivided : masterClock);

		if (burstTimeStart && firstBurstTime && cycle == HIGH && currentTime >= nextSyncPoint) { // force resync + tempo adjustment
			currentTime = nextSyncPoint; // spoof the current time so that we always remain in sync with the tempo
			triggered = triggerFirstPressed = HIGH;
			wantsEoc = true;
		}

		///// we read the values and pots and inputs, and store the time difference between ping clock and trigger
		if ((triggered == HIGH && triggerFirstPressed == HIGH)
		        || (resetPhase == true && currentTime >= tempoTic_Temp)) {
			if (wantsEoc) {
				enableEOC(currentTime);
			}
			repetitionCounter = 0;

			if (repetitionsEncoder != repetitionsEncoder_Temp) {
				repetitionsEncoder = repetitionsEncoder_Temp;
				EEPROM_write(4, repetitionsEncoder);
			}
			triggerFirstPressed = LOW;
		}

		calculateClock(currentTime); // we read the ping in and the encoder button to get : master clock, clock divided and timePortions
		readTrigger(currentTime); // we read the trigger input and the trigger button to see if it is HIGH or LOW, if it is HIGH and it is the first time it is.
		readRepetitions(currentTime); // we read the number of repetitions in the encoder, we have to attend this process often to avoid missing encoder tics.

		readDivision(currentTime);
		readDistribution(currentTime);
		readRandom(currentTime);

		handleEOC(currentTime, 30);
		handleLEDs(currentTime);

		// do this before cycle resync
		handlePulseDown(currentTime);
		handlePulseUp(currentTime);

		if (burstTimeStart && cycle == HIGH) { // CYCLE ON
			if (recycle
			        // ensure that the current cycle is really over
			        && (currentTime >= (burstTimeStart + clockDivided))
			        // ensure that the total burst (incl mult) is really over
			        && (!resync || currentTime >= (firstBurstTime + masterClock))) {
				if (resync) {
					if (repetitionsEncoder != repetitionsEncoder_Temp) {
						repetitionsEncoder = repetitionsEncoder_Temp;
						EEPROM_write(4, repetitionsEncoder);
					}
					doResync(currentTime);
				}
				startBurstInit(currentTime);
			}
		}
		else if (!burstTimeStart || cycle == LOW) {
			if (currentTime >= tempoTic_Temp) { // ensure that the timer advances
				doResync(currentTime);
			}
		}
		handleTempo(currentTime);
	}

	////////////////////
	// functions.ino functions
	////////////////////

	void doLedFlourish(int times) {
		while (times--) {
			int cur = 0;
			while (cur < 16) {
				for (int i = 0; i < 4; i++) {
					digitalWrite(ledPin[i], bitRead(cur, i));
				}
				cur++;
				delay(10);
			}
			delay(20);
		}
	}

	void doLightShow() {
		int count = 0;
		while (count < 3) {
			int cur = 15;
			while (cur >= 0) {
				for (int i = 3; i >= 0; i--) {
					digitalWrite(ledPin[i], bitRead(cur, i));
					delay(2);
				}
				delay(10);
				cur--;
			}
			cur = 0;
			while (cur < 16) {
				for (int i = 3; i >= 0; i--) {
					digitalWrite(ledPin[i], bitRead(cur, i));
					delay(2);
				}
				delay(10);
				cur++;
			}
			delay(20);
			count++;
		}
	}

	void doCalibration() {
		calibratedRepetitions = analogRead(CV_REPETITIONS);
		calibratedDistribution = analogRead(CV_DISTRIBUTION);
		calibratedDivisions = analogRead(CV_DIVISIONS);
		calibratedProbability = analogRead(CV_PROBABILITY);

		EEPROM_write(6, (calibratedRepetitions & 0xff));
		EEPROM_write(7, ((calibratedRepetitions >> 8) & 0xff));

		EEPROM_write(8, (calibratedDistribution & 0xff));
		EEPROM_write(9, ((calibratedDistribution >> 8) & 0xff));

		EEPROM_write(10, (calibratedDivisions & 0xff));
		EEPROM_write(11, ((calibratedDivisions >> 8) & 0xff));

		EEPROM_write(12, (calibratedProbability & 0xff));
		EEPROM_write(13, ((calibratedProbability >> 8) & 0xff));

		doLedFlourish(3);
	}

	bool checkCalibrationMode() {
		int buttonDown = digitalRead(TRIGGER_BUTTON);

		if (digitalRead(ENCODER_BUTTON) == 0 && buttonDown) {
			// SERIAL_PRINTLN("new calibration");
			doCalibration();
			return true;
		}
		else {
			// SERIAL_PRINTLN("reading stored calibrations");

			calibratedRepetitions = ((EEPROM_read(6) & 0xff) | ((EEPROM_read(7) << 8) & 0xff00));
			// SERIAL_PRINTLN("calibratedRepetitions: %d %04x", calibratedRepetitions, calibratedRepetitions);
			if (!calibratedRepetitions || calibratedRepetitions == 0xFFFF) calibratedRepetitions = 511;

			calibratedDistribution = ((EEPROM_read(8) & 0xff) | ((EEPROM_read(9) << 8) & 0xff00));
			// SERIAL_PRINTLN("calibratedDistribution: %d %04x", calibratedDistribution, calibratedDistribution);
			if (!calibratedDistribution || calibratedDistribution == 0xFFFF) calibratedDistribution = 511;

			calibratedDivisions = ((EEPROM_read(10) & 0xff) | ((EEPROM_read(11) << 8) & 0xff00));
			// SERIAL_PRINTLN("calibratedDivisions: %d %04x", calibratedDivisions, calibratedDivisions);
			if (!calibratedDivisions || calibratedDivisions == 0xFFFF) calibratedDivisions = 511;

			calibratedProbability = ((EEPROM_read(12) & 0xff) | ((EEPROM_read(13) << 8) & 0xff00));
			// SERIAL_PRINTLN("calibratedProbability: %d %04x", calibratedProbability, calibratedProbability);
			if (!calibratedProbability || calibratedProbability == 0xFFFF) calibratedProbability = 511;
		}
		return false;
	}

	int mapCalibratedAnalogValue(int val, int calibratedValue, int mappedValue, int start, int stop) {
		int calVal = calibratedValue - 5; // buffer of -5 from calibration
		int outVal;

		val = constrain(val, ANALOG_MIN, ANALOG_MAX); // chop 20 off of both sides
		if (val <= calVal) {
			outVal = map(val, ANALOG_MIN, calVal + 1, start, mappedValue + (start > stop ? -1 : 1)); // 32 steps
		}
		else {
			outVal = map(val, calVal, ANALOG_MAX + 1, mappedValue, stop + (start > stop ? -1 : 1));
		}
		return outVal;
	}

	void calculateClock(unsigned long now) {
		////  Read the encoder button state
		if (digitalRead(ENCODER_BUTTON) == 0) {
			bitWrite(encoderButtonState, 0, 1);
		}
		else {
			bitWrite(encoderButtonState, 0, 0);
		}
		/// Read the ping input state
		if (digitalRead(PING_STATE) == 0) {
			bitWrite(pingInState, 0, 1);
		}
		else {
			bitWrite(pingInState, 0, 0);
		}

		if (encoderButtonState == 3) {
			if (encoderLastTime && encoderButtonState == 3 && (now - encoderLastTime > 5000)) { // held longer than 5s
				if (triggerButtonPressedTime && (now - triggerButtonPressedTime > 5000)) {
					probabilityAffectsEOC = !probabilityAffectsEOC;
					EEPROM_write(15, probabilityAffectsEOC);
					triggerButtonPressedTime = 0;
				}
				else {
					retriggerMode = !retriggerMode;
					EEPROM_write(14, retriggerMode);
				}
				doLedFlourish(2);
				encoderLastTime = 0;
				encoderTapsCurrent = 0;
				encoderTapsTotal = 0;
				encoderDuration = 0;
			}
			return;
		}

		/// if ping or tap button have raised
		if ((encoderButtonState == 1) || (pingInState == 1)) {
			byte ignore = false;
			unsigned long duration;
			unsigned long average;

			if (encoderButtonState == 1) {
				duration = (now - encoderLastTime);

				// this logic is weird, but it works:
				// if we've stored a duration, test the current duration against the stored
				//    if that's an outlier, reset everything, incl. the duration, to 0 and ignore
				//    otherwise, store the duration
				// if we haven't stored a duration, check whether there's a previous time
				// (if there isn't we just started up, I guess). And if there's no previous time
				// store the previous time and ignore. If there is a previous time, store the duration.
				// Basically, this ensures that outliers start a new tap tempo collection, and that the
				// first incoming taps are properly handled.
				if ((encoderDuration &&
				        (duration > (encoderDuration * 2) || (duration < (encoderDuration >> 1))))
				        || (!(encoderDuration || encoderLastTime))) {
					ignore = true;
					encoderTapsCurrent = 0;
					encoderTapsTotal = 0; // reset collection, we had an outlier
					encoderDuration = 0;
				}
				else {
					encoderDuration = duration;
				}
				encoderLastTime = now;

				if (!ignore) {
					tempoTic_Temp = now;
					encoderTaps[encoderTapsCurrent++] = duration;
					if (encoderTapsCurrent >= TAP_TEMPO_AVG_COUNT) {
						encoderTapsCurrent = 0;
					}
					encoderTapsTotal++;
					if (encoderTapsTotal > TAP_TEMPO_AVG_COUNT) {
						encoderTapsTotal = TAP_TEMPO_AVG_COUNT;
					}
					if (encoderTapsTotal > 1) { // currently this is 2, but it could change
						average = 0;
						for (int i = 0; i < encoderTapsTotal; i++) {
							average += encoderTaps[i];
						}
						masterClock_Temp = (average / encoderTapsTotal);
					}
					else {
						masterClock_Temp = encoderTaps[0]; // should be encoderDuration
					}
					calcTimePortions();
				}

				// we could do something like -- tapping the encoder will immediately enable the encoder's last tempo
				// if it has been overridden by the ping input. So you could tap a fast tempo, override it with a slow
				// ping, pull the ping cable and then tap the encoder to switch back. Similarly, sending a single ping
				// clock would enable the last ping in tempo. In this way, it would be possible to switch between two
				// tempi. Not sure if this is a YAGNI feature, but it would be possible.

				bitWrite(encoderButtonState, 1, 1);
				EEPROM_write(0, masterClock_Temp & 0xff);
				EEPROM_write(1, (masterClock_Temp >> 8) & 0xff);
				EEPROM_write(2, (masterClock_Temp >> 16) & 0xff);
				EEPROM_write(3, (masterClock_Temp >> 24) & 0xff);
			}

			if (pingInState == 1) {
				duration = (now - pingLastTime);

				if ((pingDuration && (duration > (pingDuration * 2) || (duration < (pingDuration >> 1))))
				        || (!(pingDuration || pingLastTime))) {
					ignore = true;
					pingDuration = 0;
				}
				else {
					pingDuration = duration;
				}
				pingLastTime = now;

				if (!ignore) {
					tempoTic_Temp = now;
					masterClock_Temp = pingDuration;
					calcTimePortions();
				}
				bitWrite(pingInState, 1, 1);
			}
		}

		if (encoderButtonState == 2) {
			bitWrite(encoderButtonState, 1, 0);
		}
		if (pingInState == 2) {
			bitWrite(pingInState, 1, 0);
		}
	}

	void readTrigger(unsigned long now) {
		bool triggerCvState = digitalRead(TRIGGER_STATE);
		int buttonDown = digitalRead(TRIGGER_BUTTON);
		if (buttonDown) {
			if (triggerButtonState) { // it's a new press
				triggerButtonPressedTime = now;
			}
			else if (!encoderButtonState && triggerButtonPressedTime && (now - triggerButtonPressedTime > 5000)) {
				// toggle initial ping output
				disableFirstClock = !disableFirstClock;
				EEPROM_write(5, disableFirstClock);
				doLedFlourish(2);
				triggerButtonPressedTime = 0;
			}
		}
		else {
			triggerButtonPressedTime = 0;
		}

		triggerButtonState = !buttonDown;

		triggered = !(triggerButtonState && triggerCvState);

		if (triggered && burstTimeStart && retriggerMode == NO_RETRIGGER) {
			triggered = LOW;
			return;
		}

		if (triggered == LOW) {
			triggerFirstPressed = HIGH;
		}
		else if (triggerFirstPressed == LOW) {
			; // don't update the trigger time, we're still pressed
		}
	}

	void startBurstInit(unsigned long now) {
		burstTimeStart = now;
		burstStarted = HIGH;
		recycle = false;

		repetitionCounter = 0;
		elapsedTimeSincePrevRepetitionOld = 0;
		burstTimeAccu = 0;

		// enable the output so that we continue to count the repetitions,
		// whether we actually write to the output pin or not
		outputState = HIGH;

		int randomDif = randomPot - random(100);
		bool triggerOverride = !triggerButtonState;

		// trigger button overrides probability (unless disableFirstClock is enabled)
		silentBurst = triggerOverride ? 0 : (randomDif <= 0);
		wantsMoreBursts = triggerOverride ? HIGH : (silentBurst) ? LOW : wantsMoreBursts;

		byte firstTriggerOut = !disableFirstClock;

		digitalWrite(OUT_STATE, !firstTriggerOut);
		digitalWrite(OUT_LED, firstTriggerOut);

		switch (distributionSign) {
			case DISTRIBUTION_SIGN_POSITIVE:
				elapsedTimeSincePrevRepetitionNew = fscale(0,
				                                    clockDivided,
				                                    0,
				                                    clockDivided,
				                                    timePortions,
				                                    distribution);
				elapsedTimeSincePrevRepetition = elapsedTimeSincePrevRepetitionNew;
				break;
			case DISTRIBUTION_SIGN_NEGATIVE:
				if (repetitions > 1) {
					elapsedTimeSincePrevRepetitionOld = fscale(0,
					                                    clockDivided,
					                                    0,
					                                    clockDivided,
					                                    std::round(timePortions * repetitions),
					                                    distribution);
					elapsedTimeSincePrevRepetitionNew = fscale(0,
					                                    clockDivided,
					                                    0,
					                                    clockDivided,
					                                    std::round(timePortions * (repetitions - 1)),
					                                    distribution);
					elapsedTimeSincePrevRepetition = elapsedTimeSincePrevRepetitionOld -
					                                 elapsedTimeSincePrevRepetitionNew;
				}
				else {
					elapsedTimeSincePrevRepetitionNew = timePortions;
					elapsedTimeSincePrevRepetition = timePortions;
				}
				break;
			case DISTRIBUTION_SIGN_ZERO:
				elapsedTimeSincePrevRepetitionNew = timePortions;
				elapsedTimeSincePrevRepetition = timePortions;
				break;
		}
	}

	void readDivision(unsigned long now) {                                  //// read divisions
		int divisionsPot = analogRead(CV_DIVISIONS);
		int divisionsVal = mapCalibratedAnalogValue(divisionsPot, calibratedDivisions, 0, -4, 4);
		// SERIAL_PRINTLN("divisionsPot %d", divisionsPot);

		divisionsVal = divisionsVal + (divisionsVal > 0 ? 1 : divisionsVal < 0 ? -1 : 0); // skip 2/-2

		if (abs(divisionsPot - readDivision_divisionsPotPrev) > 5) {
			if (now >= ledQuantityTime + 350) {
				byte bitDivisions = divisionsVal == 0 ? 0 : divisionsVal < 0 ? -(divisionsVal) - 1 : (16 - divisionsVal) + 1;
				if (!burstTimeStart) {
					for (int i = 0; i < 4; i++) {
						digitalWrite(ledPin[i], bitRead(bitDivisions, i));
					}
					ledParameterTime = now;
				}
			}
			divisions_Temp = divisionsVal;
			readDivision_divisionsPotPrev = divisionsPot;
		}
	}

	void readRepetitions(unsigned long now) {
		bool encoderChanged = false;
		bool process = true;

		int16_t encNewValue = encoder_getValue();
		if (repetitionsEncoder_Temp == 1) {
			if (encNewValue <= 0) {
				process = false; // ignore it
			}
			else {
				encNewValue = 1;
			}
		}

		if (process) {
			encoderValue += encNewValue;

			if (encoderValue != lastEncoderValue) {
				lastEncoderValue = encoderValue;
				if (encoderValue >= 4) {
					encoderValue = 0;
					repetitionsEncoder_Temp++;
					encoderChanged = true;
				}
				else if (encoderValue <= -4) {
					encoderValue = 0;
					repetitionsEncoder_Temp--;
					encoderChanged = true;
				}
			}
		}

		if (repetitionsEncoder_Temp < 1) repetitionsEncoder_Temp = 1;
		if (repetitionsEncoder_Temp == 1 && encoderValue < 0) {
			lastEncoderValue = encoderValue = 0; // reset to avoid weirdness around 0
		}
		// repetitionsEncoder_Temp = constrain(repetitionsEncoder_Temp, 1, MAX_REPETITIONS);
		// SERIAL_PRINTLN("repEnc %d", repetitionsEncoder_Temp);

		int cvQuantityValue = analogRead(CV_REPETITIONS);
		cvQuantityValue = mapCalibratedAnalogValue(cvQuantityValue, calibratedRepetitions, 0, 32, -16);

		int temp = (int)repetitionsEncoder_Temp + cvQuantityValue; // it could be negative, need to cast
		repetitions_Temp = constrain(temp, 1, MAX_REPETITIONS);
		// SERIAL_PRINTLN("repTmp %d", repetitions_Temp);

		repetitionsEncoder_Temp = constrain(repetitionsEncoder_Temp, 1, MAX_REPETITIONS);

		if (repetitions_Temp != repetitionsOld) {
			if (encoderChanged || !burstTimeStart) {
				for (int i = 0; i < 4; i++) {
					digitalWrite(ledPin[i], bitRead(repetitions_Temp - 1, i));
				}
				if (encoderChanged) {
					ledQuantityTime = now;
				}
			}
			repetitionsOld = repetitions_Temp;
		}
	}

	void readRandom(unsigned long now) {
		int randomVal = analogRead(CV_PROBABILITY);
		randomVal = mapCalibratedAnalogValue(randomVal, calibratedProbability, 10, 0, 20);
		// SERIAL_PRINTLN("randomPot %d", randomPot);
		randomVal *= 5;

		if (randomVal != randomPot_Temp) {
			if (now >= ledQuantityTime + 350) {
				byte bitRandom = map(randomVal, 0, 100, 15, 0);
				if (!burstTimeStart) {
					for (int i = 0; i < 4; i++) {
						digitalWrite(ledPin[i], bitRead(bitRandom, i));
					}
					ledParameterTime = now;
				}
			}
			randomPot_Temp = randomVal;
		}
	}

	void readDistribution(unsigned long now) {
		int distributionPot = analogRead(CV_DISTRIBUTION);
		int distributionVal = mapCalibratedAnalogValue(distributionPot, calibratedDistribution, 0, -8, 8);
		// SERIAL_PRINTLN("distributionPot %d", distributionPot);

		float dist = distributionIndexArray[abs(distributionVal)];
		byte distSign = (distributionVal < 0 ? DISTRIBUTION_SIGN_NEGATIVE :
		                 distributionVal > 0 ? DISTRIBUTION_SIGN_POSITIVE :
		                 DISTRIBUTION_SIGN_ZERO);

		if (abs(distributionPot - readDistribution_distributionPotPrev) > 5 || dist != distribution_Temp) {
			if (now >= ledQuantityTime + 350) {
				byte bitDistribution = (distributionVal < 0) ? -(distributionVal) :
				                       (distributionVal > 0) ? (16 - distributionVal) : 0;

				if (!burstTimeStart) {
					for (int i = 0; i < 4; i++) {
						digitalWrite(ledPin[i], bitRead(bitDistribution, i));
					}
					ledParameterTime = now;
				}
			}

			distribution_Temp = dist;
			distributionSign_Temp = distSign;

			readDistribution_distributionPotPrev = distributionPot;
		}
	}

	void readCycle() {
		int buttonDown = digitalRead(TRIGGER_BUTTON);

		int cSwitchState = digitalRead(CYCLE_SWITCH);
		cycleInState = digitalRead(CYCLE_STATE);

		if (!cSwitchState && cycleSwitchState && buttonDown) {
			resetPhase = true;
			doLedFlourish(2);
		}
		cycleSwitchState = cSwitchState;

		if (cycleSwitchState == HIGH) {
			cycle = !cycleInState;
		}
		else {
			cycle = cycleInState;
		}
	}

	int32_t fscale(int32_t originalMin,
	               int32_t originalMax,
	               int32_t newBegin,
	               int32_t newEnd,
	               int32_t inputValue,
	               float curve) {
		int32_t originalRange = 0;
		int32_t newRange = 0;
		int32_t zeroRefCurVal = 0;
		float normalizedCurVal = 0;
		int32_t rangedValue = 0;
		bool invFlag = 0;

		// Check for out of range inputValues
		if (inputValue < originalMin) {
			inputValue = originalMin;
		}
		if (inputValue > originalMax) {
			inputValue = originalMax;
		}

		// Zero Refference the values
		originalRange = originalMax - originalMin;

		if (newEnd > newBegin) {
			newRange = newEnd - newBegin;
		}
		else {
			newRange = newBegin - newEnd;
			invFlag = 1;
		}

		zeroRefCurVal = inputValue - originalMin;
		normalizedCurVal = ((float)zeroRefCurVal / (float)originalRange);   // normalize to 0 - 1 float

		// Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
		if (originalMin > originalMax) {
			return 0;
		}

		if (invFlag == 0) {
			rangedValue =  (int32_t)((std::pow(normalizedCurVal, curve) * newRange) + newBegin);
		}
		else { // invert the ranges
			rangedValue =  (int32_t)(newBegin - (std::pow(normalizedCurVal, curve) * newRange));
		}

		return rangedValue;
	}

	void createDistributionIndex() {
		for (int i  = 0; i <= 8; i++) {
			// invert and scale in advance: positive numbers give more weight to high end on output
			// convert linear scale into logarithmic exponent for std::pow function in fscale()
			float val = mapfloat(i, 0, 8, 0, curve);
			val = val > 10. ? 10. : val < -10. ? -10. : val;
			val *= -0.1;
			val = std::pow(10, val);
			distributionIndexArray[i] = val;
		}
	}

	float mapfloat(float x, float inMin, float inMax, float outMin, float outMax) {
		return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
	}

	void handleLEDs(unsigned long now) {
		if ((now >= ledQuantityTime + 350)) {
			if ((wantsMoreBursts && !silentBurst) || (now >= ledParameterTime + 750)) {
				for (int i = 0; i < 4; i++) {
					digitalWrite(ledPin[i], bitRead((burstStarted && !silentBurst) ? repetitionCounter : repetitions_Temp - 1, i));
				}
			}
		}
	}

	void handlePulseDown(unsigned long now) {
		if ((outputState == HIGH) && (burstStarted == HIGH)) {
			if (now >= (burstTimeStart + burstTimeAccu + trigLen)) {
				outputState = LOW;
				digitalWrite(OUT_STATE, HIGH);
				digitalWrite(OUT_LED, LOW);
			}
		}
	}

	void enableEOC(unsigned long now) {
		if (inEoc) {
			eocCounter = now;
			handleEOC(now, 0); // turn off the EOC if necessary
		}

		if (!(silentBurst && probabilityAffectsEOC)) {
			// turn it (back) on
			digitalWrite(EOC_STATE, LOW);
			inEoc = true;
			eocCounter = now;
		}
		wantsEoc = false;
	}

	void handleEOC(unsigned long now, int width) {
		if (inEoc && now >= eocCounter + width) {
			digitalWrite(EOC_STATE, HIGH);
			inEoc = false;
		}
	}

	void handlePulseUp(unsigned long now) {
		int inputValue;

		// pulse up - burst time
		if ((outputState == LOW) && (burstStarted == HIGH)) {
			if (now >= (burstTimeStart + elapsedTimeSincePrevRepetition + burstTimeAccu)) { // time for a repetition
				if (repetitionCounter < repetitions - 1) { // is it the last repetition?
					outputState = HIGH;
					digitalWrite(OUT_STATE, !wantsMoreBursts);
					digitalWrite(OUT_LED, wantsMoreBursts);
					burstTimeAccu += elapsedTimeSincePrevRepetition;

					repetitionCounter++;

					switch (distributionSign) {
						case DISTRIBUTION_SIGN_POSITIVE:
							inputValue = std::round(timePortions * (repetitionCounter + 1));
							elapsedTimeSincePrevRepetitionOld = elapsedTimeSincePrevRepetitionNew;
							elapsedTimeSincePrevRepetitionNew = fscale(0,
							                                    clockDivided,
							                                    0,
							                                    clockDivided,
							                                    inputValue,
							                                    distribution);
							elapsedTimeSincePrevRepetition = elapsedTimeSincePrevRepetitionNew -
							                                 elapsedTimeSincePrevRepetitionOld;
							break;
						case DISTRIBUTION_SIGN_NEGATIVE:
							elapsedTimeSincePrevRepetitionOld = elapsedTimeSincePrevRepetitionNew;

							inputValue = std::round(timePortions * ((repetitions - 1) - repetitionCounter));
							if (!inputValue && divisions <= 0) {   // no EOC if we're dividing
								wantsEoc = true;   // we may not reach the else block below if the next trigger comes too quickly
							}
							elapsedTimeSincePrevRepetitionNew = fscale(0,
							                                    clockDivided,
							                                    0,
							                                    clockDivided,
							                                    inputValue,
							                                    distribution);
							elapsedTimeSincePrevRepetition = elapsedTimeSincePrevRepetitionOld -
							                                 elapsedTimeSincePrevRepetitionNew;
							break;
						case DISTRIBUTION_SIGN_ZERO:
							elapsedTimeSincePrevRepetitionOld = elapsedTimeSincePrevRepetitionNew;
							elapsedTimeSincePrevRepetitionNew = std::round(timePortions * (repetitionCounter + 1));
							elapsedTimeSincePrevRepetition = elapsedTimeSincePrevRepetitionNew -
							                                 elapsedTimeSincePrevRepetitionOld;
							break;
					}
				}
				else { // it's the end of the burst, but not necessarily the end of a set of bursts (mult)
					enableEOC(now);

					wantsMoreBursts = HIGH;
					burstStarted = LOW;

					readCycle();

					if (cycle) {
						recycle = true;
						divisionCounter++;
						if (divisions >= 0 || divisionCounter >= -(divisions)) {
							resync = true;
							divisionCounter = 0; // superstitious, this will also be reset in doResync()
						}
					}
					else {
						wantsMoreBursts = LOW;
						burstTimeStart = 0;
						firstBurstTime = 0;
					}
				}
			}
		}
	}

	void handleTempo(unsigned long now) {
		if (!tempoInitialized) {
			tempoTimer = now;
			tempoTimer_Temp = now + masterClock;
			tempoInitialized = true;
		}

		// using a different variable here (tempoTimer) to avoid side effects
		// when updating tempoTic. Now the tempoTimer is entirely independent
		// of the cycling, etc.

		if (!tempoStart && (now >= tempoTimer) && (now < tempoTimer + trigLen)) {
			digitalWrite(TEMPO_STATE, LOW);
			tempoStart = now;
		}
		else if (tempoStart && (int) (now - tempoStart) > trigLen) {
			digitalWrite(TEMPO_STATE, HIGH);
			tempoStart = 0;
			// DEBUG("%d", masterClock);
		}

		if (!tempoStart) { // only if we're outside of the pulse
			if (now >= tempoTimer) {
				tempoTimer = tempoTimer_Temp;
			}
			while (now >= tempoTimer) {
				tempoTimer += masterClock;
			}
		}
	}

	void doResync(unsigned long now) {
		// get the new value in advance of calctimeportions();
		divisions = divisions_Temp;
		calcTimePortions();

		// don't advance the timer unless we've reached the next tick
		// (could happen with manual trigger)
		if (now >= tempoTic_Temp) {
			tempoTic = tempoTic_Temp ? tempoTic_Temp : now;
			if (masterClock_Temp != masterClock) {
				masterClock = masterClock_Temp;
			}
			tempoTic_Temp = tempoTic + masterClock;
			tempoTimer_Temp = tempoTic_Temp;
		}

		// update other params
		repetitions = repetitions_Temp;
		clockDivided = clockDivided_Temp;
		timePortions = timePortions_Temp;
		distribution = distribution_Temp;
		distributionSign = distributionSign_Temp;
		randomPot = randomPot_Temp;

		if (timePortions < TRIGGER_LENGTH) {
			trigLen = (int)timePortions - 1;
			if (trigLen < 1) trigLen = 1;
		}
		else {
			trigLen = TRIGGER_LENGTH;
		}

		readCycle();
		resync = false;
		divisionCounter = 0; // otherwise multiple manual bursts will screw up the division count

		// time of first burst of a group of bursts
		firstBurstTime = now;
	}

	void calcTimePortions() {
		if (masterClock_Temp < 1) masterClock_Temp = 1; // ensure masterClock > 0

		if (divisions > 0) {
			clockDivided_Temp = masterClock_Temp * divisions;
		}
		else if (divisions < 0) {
			clockDivided_Temp = masterClock_Temp / (-divisions);
		}
		else if (divisions == 0) {
			clockDivided_Temp = masterClock_Temp;
		}
		timePortions_Temp = (float)clockDivided_Temp / (float)repetitions_Temp;
	}

	////////////////////
	// wrapper functions
	////////////////////

	void EEPROM_write(int address, byte value) {
		eeprom[address] = value;
	}

	byte EEPROM_read(int address) {
		return eeprom[address];
	}

	void delay(unsigned long ms) {
		// DEBUG("delay(%d) called", ms);
	}

	void updateLeds() {
		for (int i = 0; i < 16; i++) {
			lights[QUANTITY_LIGHTS + i].setBrightness(ledState == i);
		}
	}

	void digitalWrite(int pin, bool value) {
		switch (pin) {
			case TEMPO_STATE: {
				outputs[TEMPO_OUTPUT].setVoltage(value ? 10.f : 0.f);
				// TODO Make smooth light?
				lights[TEMPO_LIGHT].setBrightness(value);
			} break;
			case OUT_LED: {
				lights[OUT_LIGHT].setBrightness(value);
			} break;
			case OUT_STATE: {
				outputs[OUT_OUTPUT].setVoltage(value ? 10.f : 0.f);
			} break;
			case EOC_STATE: {
				outputs[EOC_OUTPUT].setVoltage(value ? 10.f : 0.f);
				lights[EOC_LIGHT].setBrightness(value);
			} break;
			case LED_PIN_0: {
				bitWrite(ledState, 0, value);
				updateLeds();
			} break;
			case LED_PIN_1: {
				bitWrite(ledState, 1, value);
				updateLeds();
			} break;
			case LED_PIN_2: {
				bitWrite(ledState, 2, value);
				updateLeds();
			} break;
			case LED_PIN_3: {
				bitWrite(ledState, 3, value);
				updateLeds();
			} break;
			default: return;
		}
	}

	bool digitalRead(int pin) {
		switch (pin) {
			case CYCLE_SWITCH: {
				return (int) std::round(params[CYCLE_PARAM].getValue()) == 1;
			} break;
			case CYCLE_STATE: {
				// No idea what this is. A CV input that didn't make it to production?
				return false;
			} break;
			case PING_STATE: {
				return inputs[PING_INPUT].getVoltage() >= 2.f;
			} break;
			case ENCODER_BUTTON: {
				// TODO
				return false;
			} break;
			case ENCODER_1:
			case ENCODER_2: {
				// Used by the hardware to detect encoder clicks, not needed in software.
				return false;
			} break;
			case TRIGGER_STATE: {
				return !(inputs[TRIGGER_INPUT].getVoltage() >= 2.f);
			} break;
			case TRIGGER_BUTTON: {
				return (int) std::round(params[TRIGGER_PARAM].getValue()) == 1;
			} break;
			default: return false;
		}
	}

	int analogRead(int pin) {
		switch (pin) {
			case CV_DIVISIONS: {
				return (int) (clamp(params[TIME_PARAM].getValue() + inputs[TIME_INPUT].getVoltage() / 10.f, 0.f, 1.f) * 1023);
			} break;
			case CV_PROBABILITY: {
				return (int) (clamp(params[PROBABILITY_PARAM].getValue() + inputs[PROBABILITY_INPUT].getVoltage() / 10.f, 0.f, 1.f) * 1023);
			} break;
			case CV_REPETITIONS: {
				// Note that this is multiplicative instead of additive like the others.
				return (int) (clamp(params[QUANTITY_CV_PARAM].getValue() * inputs[QUANTITY_INPUT].getVoltage() / 10.f + 0.5f, 0.f, 1.f) * 1023);
			} break;
			case CV_DISTRIBUTION: {
				return (int) (clamp(params[DISTRIBUTION_PARAM].getValue() + inputs[DISTRIBUTION_INPUT].getVoltage() / 10.f, 0.f, 1.f) * 1023);
			} break;
			default: return 0;
		}
	}

	int encoder_getValue() {
		const float scale = 40.f;
		float quantityValue = scale * params[QUANTITY_PARAM].getValue();
		float delta = quantityValue - quantityValueLast;
		quantityValueLast = std::round(quantityValue);
		return (int) std::round(delta);
	}
};


struct BurstWidget : ModuleWidget {
	BurstWidget(Burst *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Burst.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<BefacoSwitch>(mm2px(Vec(28.44228, 10.13642)), module, Burst::CYCLE_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(mm2px(Vec(9.0322, 16.21467)), module, Burst::QUANTITY_PARAM));
		addParam(createParam<BefacoPush>(mm2px(Vec(28.43253, 29.6592)), module, Burst::TRIGGER_PARAM));
		addParam(createParam<BefacoTinyKnob>(mm2px(Vec(17.26197, 41.95461)), module, Burst::QUANTITY_CV_PARAM));
		addParam(createParam<BefacoTinyKnob>(mm2px(Vec(22.85243, 58.45676)), module, Burst::DISTRIBUTION_PARAM));
		addParam(createParam<BefacoTinyKnob>(mm2px(Vec(28.47229, 74.91607)), module, Burst::TIME_PARAM));
		addParam(createParam<BefacoTinyKnob>(mm2px(Vec(22.75115, 91.35201)), module, Burst::PROBABILITY_PARAM));

		addInput(createInput<PJ301MPort>(mm2px(Vec(2.02153, 42.27628)), module, Burst::QUANTITY_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(7.90118, 58.74959)), module, Burst::DISTRIBUTION_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(2.05023, 75.25163)), module, Burst::PING_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(13.7751, 75.23049)), module, Burst::TIME_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(7.89545, 91.66642)), module, Burst::PROBABILITY_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(1.11155, 109.30346)), module, Burst::TRIGGER_INPUT));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(11.07808, 109.30346)), module, Burst::TEMPO_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(21.08452, 109.32528)), module, Burst::EOC_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(31.01113, 109.30346)), module, Burst::OUT_OUTPUT));

		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.03676, 9.98712)), module, Burst::QUANTITY_LIGHTS + 0));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(18.35846, 10.85879)), module, Burst::QUANTITY_LIGHTS + 1));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(22.05722, 13.31827)), module, Burst::QUANTITY_LIGHTS + 2));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.48707, 16.96393)), module, Burst::QUANTITY_LIGHTS + 3));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(25.38476, 21.2523)), module, Burst::QUANTITY_LIGHTS + 4));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.48707, 25.5354)), module, Burst::QUANTITY_LIGHTS + 5));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(22.05722, 29.16905)), module, Burst::QUANTITY_LIGHTS + 6));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(18.35846, 31.62236)), module, Burst::QUANTITY_LIGHTS + 7));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.03676, 32.48786)), module, Burst::QUANTITY_LIGHTS + 8));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(9.74323, 31.62236)), module, Burst::QUANTITY_LIGHTS + 9));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(6.10149, 29.16905)), module, Burst::QUANTITY_LIGHTS + 10));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(3.68523, 25.5354)), module, Burst::QUANTITY_LIGHTS + 11));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(2.85312, 21.2523)), module, Burst::QUANTITY_LIGHTS + 12));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(3.68523, 16.96393)), module, Burst::QUANTITY_LIGHTS + 13));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(6.10149, 13.31827)), module, Burst::QUANTITY_LIGHTS + 14));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(9.74323, 10.85879)), module, Burst::QUANTITY_LIGHTS + 15));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.18119, 104.2831)), module, Burst::TEMPO_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.14772, 104.2831)), module, Burst::EOC_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(34.11425, 104.2831)), module, Burst::OUT_LIGHT));
	}
};


Model *modelBurst = createModel<Burst, BurstWidget>("Burst");

