#pragma once

#include "NoisePlethoraPlugin.hpp"



class arrayOnTheRocks: public NoisePlethoraPlugin {
public:

	arrayOnTheRocks()
	// : patchCord1(waveform1, 0, waveformMod1, 0)
	{}
	~arrayOnTheRocks() override {}

	// delete copy constructors
	arrayOnTheRocks(const arrayOnTheRocks&) = delete;
	arrayOnTheRocks& operator=(const arrayOnTheRocks&) = delete;


	void init() override {

		//noise1.amplitude(1);

		waveformMod1.arbitraryWaveform(myWaveform, 172.0);

		waveformMod1.begin(1, 250, WAVEFORM_ARBITRARY);
		waveform1.begin(1, 500, WAVEFORM_SINE);

		/*test=random(-28000, 28000);

		myWaveform = {0,  1895,  3748,  5545,  7278,  8934, 10506, 11984, 13362, 14634,
		 test, 16840, 17769, 18580, 19274, 19853, 20319, 20678, 20933, 21093,
		 21163, 21153, 21072, 20927, 20731, 20492, 20221, test, 19625, 19320,
		 19022, 18741, 18486, 18263, 18080, 17942, 17853, 17819, 17841, 17920,
		 18058, 18254, 18507, 18813, 19170, 19573, 20017, 20497, 21006, test,
		 test, test, test, 23753, 24294, 24816, 25314, 25781, 26212, 26604,
		 26953, test, test, 27718, 27876, 27986, test, test, test, 27989,
		 27899, 27782, 27644, 27490, test, test, test, test, test, 26582,
		 26487, 26423, test, test, test, test, test, 26812, 27012, 27248,
		 27514, 27808, 28122, test, 28787, test, 29451, 29762, 30045, 30293,
		 test, 30643, 30727, 30738, 30667, test, 30254, 29897, test, 28858,
		 28169, 27363, 26441, 25403, 24251, 22988, 21620, 20150, 18587, 16939,
		 15214, 13423, 11577,  9686,  7763,  5820,  3870,  1926,     0, -1895,
		 -3748, -5545, -7278, -8934,-10506, test,-13362,-14634,-15794,-16840,
		-17769,-18580,-19274,-19853,-20319,-20678,-20933,-21093,-21163,-21153,
		-21072,-20927,-test,-20492,-20221,-19929,-19625,-19320,-19022,-18741,
		-test,-18263,-18080,-17942,-17853,-test,-17841,-17920,-18058,-18254,
		-18507,-18813,-19170,-19573,-test,-20497,-21006,-21538,-22085,-22642,
		-23200,-23753,-test,-test,-25314,-25781,-26212,-26604,-26953,-27256,
		-27511,-27718,-test,-test,-test,-28068,-28047,-test,-27899,-27782,
		-27644,-27490,-27326,-test,-26996,-26841,-26701,-26582,-26487,-test,
		-26392,-26397,-26441,-26525,-test,-26812,-test,-27248,-27514,-27808,
		-28122,-28451,-test,-test,-test,-29762,-test,-test,-test,-test,
		-test,-test,-test,-test,-test,-test,-test,test,-28169,-27363,
		-test,-test,-test,-test,-test,-test,-test,-test,-test,test,
		-test, -test, -test, -test, -test, -test};*/
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);
		// float pitch2 = pow(knob_2, 2);

		waveformMod1.frequency(10 + (pitch1 * 10000));
		waveform1.frequency(100 + (pitch1 * 500));

		waveform1.amplitude(knob_2);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		waveform1.update(&waveformBlock);
		waveformMod1.update(&waveformBlock, nullptr, &waveformModBlock);

		blockBuffer.pushBuffer(waveformModBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return waveformMod1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformBlock, waveformModBlock;

	// GUItool: begin automatically generated code
	AudioSynthWaveform       waveform1;      //xy=305.3333282470703,648.3333358764648
	AudioSynthWaveformModulated waveformMod1;   //xy=475.88893127441406,517.2221565246582
	// AudioConnection          patchCord1;

	const int16_t test = 8622; // (int16_t)random(-28000, 28000);
	const int16_t myWaveform[256] = {0,  1895,  3748,  5545,  7278,  8934, 10506, 11984, 13362, 14634,
	                                 test, 16840, 17769, 18580, 19274, 19853, 20319, 20678, 20933, 21093,
	                                 21163, 21153, 21072, 20927, 20731, 20492, 20221, test, 19625, 19320,
	                                 19022, 18741, 18486, 18263, 18080, 17942, 17853, 17819, 17841, 17920,
	                                 18058, 18254, 18507, 18813, 19170, 19573, 20017, 20497, 21006, test,
	                                 test, test, test, 23753, 24294, 24816, 25314, 25781, 26212, 26604,
	                                 26953, test, test, 27718, 27876, 27986, test, test, test, 27989,
	                                 27899, 27782, 27644, 27490, test, test, test, test, test, 26582,
	                                 26487, 26423, test, test, test, test, test, 26812, 27012, 27248,
	                                 27514, 27808, 28122, test, 28787, test, 29451, 29762, 30045, 30293,
	                                 test, 30643, 30727, 30738, 30667, test, 30254, 29897, test, 28858,
	                                 28169, 27363, 26441, 25403, 24251, 22988, 21620, 20150, 18587, 16939,
	                                 15214, 13423, 11577,  9686,  7763,  5820,  3870,  1926,     0, -1895,
	                                 -3748, -5545, -7278, -8934, -10506, test, -13362, -14634, -15794, -16840,
	                                 -17769, -18580, -19274, -19853, -20319, -20678, -20933, -21093, -21163, -21153,
	                                 -21072, -20927, (int16_t) - test, -20492, -20221, -19929, -19625, -19320, -19022, -18741,
	                                 (int16_t) - test, -18263, -18080, -17942, -17853, (int16_t) - test, -17841, -17920, -18058, -18254,
	                                 -18507, -18813, -19170, -19573, (int16_t) - test, -20497, -21006, -21538, -22085, -22642,
	                                 -23200, -23753, (int16_t) - test, (int16_t) - test, -25314, -25781, -26212, -26604, -26953, -27256,
	                                 -27511, -27718, (int16_t) - test, (int16_t) - test, (int16_t) - test, -28068, -28047, (int16_t) - test, -27899, -27782,
	                                 -27644, -27490, -27326, (int16_t) - test, -26996, -26841, -26701, -26582, -26487, (int16_t) - test,
	                                 -26392, -26397, -26441, -26525, (int16_t) - test, -26812, (int16_t) - test, -27248, -27514, -27808,
	                                 -28122, -28451, (int16_t) - test, (int16_t) - test, (int16_t) - test, -29762, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test,
	                                 (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, test, -28169, -27363,
	                                 (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, test,
	                                 (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test, (int16_t) - test
	                                };

	// int current_waveform = 0;

	/* PREGUNTA A DIEGO ¿Este sobra? */
	//extern const int16_t myWaveform[256];  // defined in myWaveform.ino


};






REGISTER_PLUGIN(arrayOnTheRocks); // this is important, so that we can include the plugin in a bank
