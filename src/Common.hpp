#pragma once

#include <rack.hpp>
#include <dsp/common.hpp>

struct BefacoTinyKnobRed : app::SvgKnob {
	BefacoTinyKnobRed() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoTinyKnobRed.svg")));
	}
};

struct BefacoTinyKnobWhite : app::SvgKnob {
	BefacoTinyKnobWhite() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::system("res/ComponentLibrary/BefacoTinyKnob.svg")));
	}
};

struct BefacoOutputPort : app::SvgPort {
	BefacoOutputPort() {		
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoOutputPort.svg")));
	}
};

struct BefacoInputPort : app::SvgPort {
	BefacoInputPort() {		
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoInputPort.svg")));
	}
};

/**
    High-order filter to be used for anti-aliasing or anti-imaging.
    The template parameter N should be 1/2 the desired filter order.

    Currently uses an 2*N-th order Butterworth filter.
    @TODO: implement Chebyshev, Elliptic filter options.
*/
template<int N>
class AAFilter {
public:
	AAFilter() = default;

	/** Calculate Q values for a Butterworth filter of a given order */
	static std::vector<float> calculateButterQs(int order) {
		const int lim = int (order / 2);
		std::vector<float> Qs;

		for (int k = 1; k <= lim; ++k) {
			auto b = -2.0f * std::cos((2.0f * k + order - 1) * 3.14159 / (2.0f * order));
			Qs.push_back(1.0f / b);
		}

		std::reverse(Qs.begin(), Qs.end());
		return Qs;
	}

	/**
	 * Resets the filter to process at a new sample rate.
	 *
	 * @param sampleRate: The base (i.e. pre-oversampling) sample rate of the audio being processed
	 * @param osRatio: The oversampling ratio at which the filter is being used
	 */
	void reset(float sampleRate, int osRatio) {
		float fc = 0.98f * (sampleRate / 2.0f);
		auto Qs = calculateButterQs(2 * N);

		for (int i = 0; i < N; ++i)
			filters[i].setParameters(dsp::BiquadFilter::Type::LOWPASS, fc / (osRatio * sampleRate), Qs[i], 1.0f);
	}

	inline float process(float x) noexcept {
		for (int i = 0; i < N; ++i)
			x = filters[i].process(x);

		return x;
	}

private:
	dsp::BiquadFilter filters[N];
};



/**
 * Base class for oversampling of any order
 */
class BaseOversampling {
public:
	BaseOversampling() = default;
	virtual ~BaseOversampling() {}

	/** Resets the oversampler for processing at some base sample rate */
	virtual void reset(float /*baseSampleRate*/) = 0;

	/** Upsample a single input sample and update the oversampled buffer */
	virtual void upsample(float) noexcept = 0;

	/** Output a downsampled output sample from the current oversampled buffer */
	virtual float downsample() noexcept = 0;

	/** Returns a pointer to the oversampled buffer */
	virtual float* getOSBuffer() noexcept = 0;
};

/**
    Class to implement an oversampled process.
    To use, create an object and prepare using `reset()`.

    Then use the following code to process samples:
    @code
    oversample.upsample(x);
    for(int k = 0; k < ratio; k++)
        oversample.osBuffer[k] = processSample(oversample.osBuffer[k]);
    float y = oversample.downsample();
    @endcode
*/
template<int ratio, int filtN = 4>
class Oversampling : public BaseOversampling {
public:
	Oversampling() = default;
	virtual ~Oversampling() {}

	void reset(float baseSampleRate) override {
		aaFilter.reset(baseSampleRate, ratio);
		aiFilter.reset(baseSampleRate, ratio);
		std::fill(osBuffer, &osBuffer[ratio], 0.0f);
	}

	inline void upsample(float x) noexcept override {
		osBuffer[0] = ratio * x;
		std::fill(&osBuffer[1], &osBuffer[ratio], 0.0f);

		for (int k = 0; k < ratio; k++)
			osBuffer[k] = aiFilter.process(osBuffer[k]);
	}

	inline float downsample() noexcept override {
		float y = 0.0f;
		for (int k = 0; k < ratio; k++)
			y = aaFilter.process(osBuffer[k]);

		return y;
	}

	inline float* getOSBuffer() noexcept override {
		return osBuffer;
	}

	float osBuffer[ratio];

private:
	AAFilter<filtN> aaFilter; // anti-aliasing filter
	AAFilter<filtN> aiFilter; // anti-imaging filter
};


/**
    Class to implement an oversampled process, with variable
    oversampling factor. To use, create an object, set the oversampling
    factor using `setOversamplingindex()` and prepare using `reset()`.

    Then use the following code to process samples:
    @code
    oversample.upsample(x);
    float* osBuffer = oversample.getOSBuffer();
    for(int k = 0; k < ratio; k++)
        osBuffer[k] = processSample(osBuffer[k]);
    float y = oversample.downsample();
    @endcode
*/
template<int filtN = 4>
class VariableOversampling {
public:
	VariableOversampling() = default;

	/** Prepare the oversampler to process audio at a given sample rate */
	void reset(float sampleRate) {
		for (auto* os : oss)
			os->reset(sampleRate);
	}

	/** Sets the oversampling factor as 2^idx */
	void setOversamplingIndex(int newIdx) {
		osIdx = newIdx;
	}

	/** Returns the oversampling index */
	int getOversamplingIndex() const noexcept {
		return osIdx;
	}

	/** Upsample a single input sample and update the oversampled buffer */
	inline void upsample(float x) noexcept {
		oss[osIdx]->upsample(x);
	}

	/** Output a downsampled output sample from the current oversampled buffer */
	inline float downsample() noexcept {
		return oss[osIdx]->downsample();
	}

	/** Returns a pointer to the oversampled buffer */
	inline float* getOSBuffer() noexcept {
		return oss[osIdx]->getOSBuffer();
	}

	/** Returns the current oversampling factor */
	int getOversamplingRatio() const noexcept {
		return 1 << osIdx;
	}


private:
	enum {
		NumOS = 5, // number of oversampling options
	};

	int osIdx = 0;

	Oversampling < 1 << 0, filtN > os0; // 1x
	Oversampling < 1 << 1, filtN > os1; // 2x
	Oversampling < 1 << 2, filtN > os2; // 4x
	Oversampling < 1 << 3, filtN > os3; // 8x
	Oversampling < 1 << 4, filtN > os4; // 16x
	BaseOversampling* oss[NumOS] = { &os0, &os1, &os2, &os3, &os4 };

};
