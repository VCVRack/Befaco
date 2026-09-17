#include "Random8Core.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

struct Random8Core::Impl {
    std::array<R8::Channel, R8::NUM_CHANNELS> channels{};
    std::array<uint16_t, R8::NUM_CHANNELS> outputs{};
    float controlAccumulator = 0.0f;
};

namespace {
// The Pico firmware updates all channels and writes both DACs from the main loop,
// with observed loop deltas around 1.4-2.4 ms in the hardware source comments.
constexpr float kHardwareControlPeriodSeconds = 0.0015f;
constexpr float kMaxAccumulatedProcessSeconds = 0.1f;

R8::ChannelData makeDefaultChannelData(int channel) {
    R8::ChannelData data;
    data.coreInfo.style = R8::RandomStyle::Standard;
    data.coreInfo.seed = static_cast<uint8_t>(std::rand() & 0xff);
    data.myIndex = static_cast<uint8_t>(channel);
    data.isLooping = 0;
    data.loopSteps = 16;
    data.scale = 1;
    data.probability = 100;
    data.divider = 1;
    data.slide = 0;
    data.atOffset = 0;
    data.attenuation = R8::SIXTEEN_BIT_MAX;
    return data;
}
} // namespace

Random8Core::Random8Core() : impl_(std::make_unique<Impl>()) { reset(); }

Random8Core::Random8Core(Random8Core &&other) noexcept = default;
Random8Core &Random8Core::operator=(Random8Core &&other) noexcept = default;
Random8Core::~Random8Core() = default;

void Random8Core::reset() {
    impl_->controlAccumulator = 0.0f;
    for (int i = 0; i < R8::NUM_CHANNELS; i++) {
        impl_->channels[i].setup(makeDefaultChannelData(i));
        impl_->outputs[i] = 0;
    }
}

void Random8Core::resetChannel(int channel) {
    if (channel < 0 || channel >= R8::NUM_CHANNELS) {
        return;
    }

    impl_->channels[channel].setup(makeDefaultChannelData(channel));
    impl_->outputs[channel] = 0;
}

R8::Channel::ChannelState Random8Core::getChannelState(int channel) const {
    R8::Channel::ChannelState out;
    if (channel < 0 || channel >= R8::NUM_CHANNELS) {
        return out;
    }

    return impl_->channels[channel].getState();
}

void Random8Core::setChannelState(int channel, const R8::Channel::ChannelState &state) {
    if (channel < 0 || channel >= R8::NUM_CHANNELS) {
        return;
    }

    impl_->channels[channel].setState(state);
    impl_->outputs[channel] = state.currentVal;
    impl_->controlAccumulator = 0.0f;
}

void Random8Core::setChannelParams(int channel, const Random8ChannelParams &params) {
    if (channel < 0 || channel >= R8::NUM_CHANNELS) {
        return;
    }

    R8::Channel &ch = impl_->channels[channel];
    const int nextStyle = std::clamp(params.style, 0, R8::NUM_AVAILABLE_STYLES - 1);
    if (ch.data.coreInfo.style != nextStyle) {
        ch.data.coreInfo.style = static_cast<R8::RandomStyle>(nextStyle);
        ch.setCore();
    }

    const int nextScale = std::clamp(params.scale, 0, R8::NUM_AVAILABLE_SCALES - 1);
    if (ch.data.scale != nextScale) {
        ch.setScale(static_cast<uint8_t>(nextScale));
    }

    ch.data.divider = static_cast<uint8_t>(std::max(1, params.divider));
    ch.data.probability = static_cast<uint8_t>(std::clamp(params.probability, 0, 100));
    ch.data.loopSteps = static_cast<uint8_t>(std::clamp(params.steps, 1, 32));
    ch.data.isLooping = static_cast<uint8_t>(std::clamp(params.loopMode, 0, 2));

    ch.data.attenuation = static_cast<uint16_t>(std::round(std::clamp(params.attenuation, 0.0f, 1.0f) * R8::SIXTEEN_BIT_MAX));
    ch.data.atOffset = static_cast<uint16_t>(std::round(std::clamp(params.offset, 0.0f, 1.0f) * R8::SIXTEEN_BIT_MAX));
    ch.data.slide = static_cast<uint16_t>(std::round(std::clamp(params.slide, 0.0f, 1.0f) * 1000.0f));
}

void Random8Core::onTrigger(int channel) {
    if (channel < 0 || channel >= R8::NUM_CHANNELS) {
        return;
    }

    R8::Channel &ch = impl_->channels[channel];
    if (ch.should_trigger()) {
        ch.copyNextIntoCurrent();
    }
    ch.tick();
}

void Random8Core::process(float deltaSeconds) {
    if (deltaSeconds <= 0.0f) {
        return;
    }

    impl_->controlAccumulator += std::min(deltaSeconds, kMaxAccumulatedProcessSeconds);
    while (impl_->controlAccumulator >= kHardwareControlPeriodSeconds) {
        for (int i = 0; i < R8::NUM_CHANNELS; i++) {
            impl_->outputs[i] =
                impl_->channels[i].process_val(impl_->channels[i].data.currentVal, kHardwareControlPeriodSeconds);
        }
        impl_->controlAccumulator -= kHardwareControlPeriodSeconds;
    }
}

float Random8Core::getOutputVolts(int channel) const {
    if (channel < 0 || channel >= R8::NUM_CHANNELS) {
        return 0.0f;
    }

    return (static_cast<float>(impl_->outputs[channel]) / R8::SIXTEEN_BIT_MAX) * 10.0f;
}
