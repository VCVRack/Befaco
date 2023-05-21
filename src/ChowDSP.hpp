#pragma once
#include <rack.hpp>


namespace chowdsp {
	// code taken from https://github.com/jatinchowdhury18/ChowDSP-VCV/blob/master/src/shared/, commit 21701fb 
	// * AAFilter.hpp
	// * VariableOversampling.hpp
	// * oversampling.hpp
	// * iir.hpp

template <int ORDER, typename T = float>
struct IIRFilter {
	/** transfer function numerator coefficients: b_0, b_1, etc.*/
	T b[ORDER] = {};

	/** transfer function denominator coefficients: a_0, a_1, etc.*/
	T a[ORDER] = {};

	/** filter state */
	T z[ORDER];

	IIRFilter() {
		reset();
	}

	void reset() {
		std::fill(z, &z[ORDER], 0.0f);
	}

	void setCoefficients(const T* b, const T* a) {
		for (int i = 0; i < ORDER; i++) {
			this->b[i] = b[i];
		}
		for (int i = 1; i < ORDER; i++) {
			this->a[i] = a[i];
		}
	}

	template <int N = ORDER>
	inline typename std::enable_if <N == 2, T>::type process(T x) noexcept {
		T y = z[1] + x * b[0];
		z[1] = x * b[1] - y * a[1];
		return y;
	}

	template <int N = ORDER>
	inline typename std::enable_if <N == 3, T>::type process(T x) noexcept {
		T y = z[1] + x * b[0];
		z[1] = z[2] + x * b[1] - y * a[1];
		z[2] = x * b[2] - y * a[2];
		return y;
	}

	template <int N = ORDER>
	inline typename std::enable_if < (N > 3), T >::type process(T x) noexcept {
		T y = z[1] + x * b[0];

		for (int i = 1; i < ORDER - 1; ++i)
			z[i] = z[i + 1] + x * b[i] - y * a[i];

		z[ORDER - 1] = x * b[ORDER - 1] - y * a[ORDER - 1];

		return y;
	}

	/** Computes the complex transfer function $H(s)$ at a particular frequency
	s: normalized angular frequency equal to $2 \pi f / f_{sr}$ ($\pi$ is the Nyquist frequency)
	*/
	std::complex<T> getTransferFunction(T s) {
		// Compute sum(a_k z^-k) / sum(b_k z^-k) where z = e^(i s)
		std::complex<T> bSum(b[0], 0);
		std::complex<T> aSum(1, 0);
		for (int i = 1; i < ORDER; i++) {
			T p = -i * s;
			std::complex<T> z(simd::cos(p), simd::sin(p));
			bSum += b[i] * z;
			aSum += a[i - 1] * z;
		}
		return bSum / aSum;
	}

	T getFrequencyResponse(T f) {
		return simd::abs(getTransferFunction(2 * M_PI * f));
	}

	T getFrequencyPhase(T f) {
		return simd::arg(getTransferFunction(2 * M_PI * f));
	}
};

template <typename T = float>
struct TBiquadFilter : IIRFilter<3, T> {
	enum Type {
		LOWPASS,
		HIGHPASS,
		LOWSHELF,
		HIGHSHELF,
		BANDPASS,
		PEAK,
		NOTCH,
		NUM_TYPES
	};

	TBiquadFilter() {
		setParameters(LOWPASS, 0.f, 0.f, 1.f);
	}

	/** Calculates and sets the biquad transfer function coefficients.
	f: normalized frequency (cutoff frequency / sample rate), must be less than 0.5
	Q: quality factor
	V: gain
	*/
	void setParameters(Type type, float f, float Q, float V) {
		float K = std::tan(M_PI * f);
		switch (type) {
			case LOWPASS: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = K * K * norm;
				this->b[1] = 2.f * this->b[0];
				this->b[2] = this->b[0];
				this->a[1] = 2.f * (K * K - 1.f) * norm;
				this->a[2] = (1.f - K / Q + K * K) * norm;
			} break;

			case HIGHPASS: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = norm;
				this->b[1] = -2.f * this->b[0];
				this->b[2] = this->b[0];
				this->a[1] = 2.f * (K * K - 1.f) * norm;
				this->a[2] = (1.f - K / Q + K * K) * norm;

			} break;

			case LOWSHELF: {
				float sqrtV = std::sqrt(V);
				if (V >= 1.f) {
					float norm = 1.f / (1.f + M_SQRT2 * K + K * K);
					this->b[0] = (1.f + M_SQRT2 * sqrtV * K + V * K * K) * norm;
					this->b[1] = 2.f * (V * K * K - 1.f) * norm;
					this->b[2] = (1.f - M_SQRT2 * sqrtV * K + V * K * K) * norm;
					this->a[1] = 2.f * (K * K - 1.f) * norm;
					this->a[2] = (1.f - M_SQRT2 * K + K * K) * norm;
				}
				else {
					float norm = 1.f / (1.f + M_SQRT2 / sqrtV * K + K * K / V);
					this->b[0] = (1.f + M_SQRT2 * K + K * K) * norm;
					this->b[1] = 2.f * (K * K - 1) * norm;
					this->b[2] = (1.f - M_SQRT2 * K + K * K) * norm;
					this->a[1] = 2.f * (K * K / V - 1.f) * norm;
					this->a[2] = (1.f - M_SQRT2 / sqrtV * K + K * K / V) * norm;
				}
			} break;

			case HIGHSHELF: {
				float sqrtV = std::sqrt(V);
				if (V >= 1.f) {
					float norm = 1.f / (1.f + M_SQRT2 * K + K * K);
					this->b[0] = (V + M_SQRT2 * sqrtV * K + K * K) * norm;
					this->b[1] = 2.f * (K * K - V) * norm;
					this->b[2] = (V - M_SQRT2 * sqrtV * K + K * K) * norm;
					this->a[1] = 2.f * (K * K - 1.f) * norm;
					this->a[2] = (1.f - M_SQRT2 * K + K * K) * norm;
				}
				else {
					float norm = 1.f / (1.f / V + M_SQRT2 / sqrtV * K + K * K);
					this->b[0] = (1.f + M_SQRT2 * K + K * K) * norm;
					this->b[1] = 2.f * (K * K - 1.f) * norm;
					this->b[2] = (1.f - M_SQRT2 * K + K * K) * norm;
					this->a[1] = 2.f * (K * K - 1.f / V) * norm;
					this->a[2] = (1.f / V - M_SQRT2 / sqrtV * K + K * K) * norm;
				}
			} break;

			case BANDPASS: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = K / Q * norm;
				this->b[1] = 0.f;
				this->b[2] = -this->b[0];
				this->a[1] = 2.f * (K * K - 1.f) * norm;
				this->a[2] = (1.f - K / Q + K * K) * norm;
			} break;

			case PEAK: {
				float c = 1.0f / K;
				float phi = c * c;
				float Knum = c / Q;
				float Kdenom = Knum;

				if (V > 1.0f)
					Knum *= V;
				else
					Kdenom /= V;

				float norm = phi + Kdenom + 1.0;
				this->b[0] = (phi + Knum + 1.0f) / norm;
				this->b[1] = 2.0f * (1.0f - phi) / norm;
				this->b[2] = (phi - Knum + 1.0f) / norm;
				this->a[1] = 2.0f * (1.0f - phi) / norm;
				this->a[2] = (phi - Kdenom + 1.0f) / norm;
			} break;

			case NOTCH: {
				float norm = 1.f / (1.f + K / Q + K * K);
				this->b[0] = (1.f + K * K) * norm;
				this->b[1] = 2.f * (K * K - 1.f) * norm;
				this->b[2] = this->b[0];
				this->a[1] = this->b[1];
				this->a[2] = (1.f - K / Q + K * K) * norm;
			} break;

			default: break;
		}
	}
};

typedef TBiquadFilter<> BiquadFilter;


/**
    High-order filter to be used for anti-aliasing or anti-imaging.
    The template parameter N should be 1/2 the desired filter order.

    Currently uses an 2*N-th order Butterworth filter.
    source: https://github.com/jatinchowdhury18/ChowDSP-VCV/blob/master/src/shared/AAFilter.hpp
*/
template<int N, typename T>
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
		float fc = 0.85f * (sampleRate / 2.0f);
		auto Qs = calculateButterQs(2 * N);

		for (int i = 0; i < N; ++i)
			filters[i].setParameters(TBiquadFilter<T>::Type::LOWPASS, fc / (osRatio * sampleRate), Qs[i], 1.0f);
	}

	inline T process(T x) noexcept {
		for (int i = 0; i < N; ++i)
			x = filters[i].process(x);

		return x;
	}

private:
	TBiquadFilter<T> filters[N];
};



/**
 * Base class for oversampling of any order
 * source: https://github.com/jatinchowdhury18/ChowDSP-VCV/blob/master/src/shared/oversampling.hpp
 */
template<typename T>
class BaseOversampling {
public:
	BaseOversampling() = default;
	virtual ~BaseOversampling() {}

	/** Resets the oversampler for processing at some base sample rate */
	virtual void reset(float /*baseSampleRate*/) = 0;

	/** Upsample a single input sample and update the oversampled buffer */
	virtual void upsample(T) noexcept = 0;

	/** Output a downsampled output sample from the current oversampled buffer */
	virtual T downsample() noexcept = 0;

	/** Returns a pointer to the oversampled buffer */
	virtual T* getOSBuffer() noexcept = 0;
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
template<int ratio, int filtN = 4, typename T = float>
class Oversampling : public BaseOversampling<T> {
public:
	Oversampling() = default;
	virtual ~Oversampling() {}

	void reset(float baseSampleRate) override {
		aaFilter.reset(baseSampleRate, ratio);
		aiFilter.reset(baseSampleRate, ratio);
		std::fill(osBuffer, &osBuffer[ratio], 0.0f);
	}

	inline void upsample(T x) noexcept override {
		osBuffer[0] = ratio * x;
		std::fill(&osBuffer[1], &osBuffer[ratio], 0.0f);

		for (int k = 0; k < ratio; k++)
			osBuffer[k] = aiFilter.process(osBuffer[k]);
	}

	inline T downsample() noexcept override {
		T y = 0.0f;
		for (int k = 0; k < ratio; k++)
			y = aaFilter.process(osBuffer[k]);

		return y;
	}

	inline T* getOSBuffer() noexcept override {
		return osBuffer;
	}

	T osBuffer[ratio];

private:
	AAFilter<filtN, T> aaFilter; // anti-aliasing filter
	AAFilter<filtN, T> aiFilter; // anti-imaging filter
};

typedef Oversampling<1, 4, simd::float_4> OversamplingSIMD;


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

	source (modified): https://github.com/jatinchowdhury18/ChowDSP-VCV/blob/master/src/shared/VariableOversampling.hpp
*/
template<int filtN = 4, typename T = float>
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
	inline void upsample(T x) noexcept {
		oss[osIdx]->upsample(x);
	}

	/** Output a downsampled output sample from the current oversampled buffer */
	inline T downsample() noexcept {
		return oss[osIdx]->downsample();
	}

	/** Returns a pointer to the oversampled buffer */
	inline T* getOSBuffer() noexcept {
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

	Oversampling < 1 << 0, filtN, T > os0; // 1x
	Oversampling < 1 << 1, filtN, T > os1; // 2x
	Oversampling < 1 << 2, filtN, T > os2; // 4x
	Oversampling < 1 << 3, filtN, T > os3; // 8x
	Oversampling < 1 << 4, filtN, T > os4; // 16x
	BaseOversampling<T>* oss[NumOS] = { &os0, &os1, &os2, &os3, &os4 };
};

} // namespace chowdsp
