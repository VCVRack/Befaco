#pragma once

#include <memory>

#include "r8_channel_vcv.h"

struct Random8ChannelParams {
    float attenuation = 1.0f;
    float offset = 0.0f;
    int scale = 1;
    int style = 0;
    int divider = 1;
    int probability = 100;
    int steps = 16;
    int loopMode = 0;
    float slide = 0.0f;
};

class Random8Core {
  public:
    Random8Core();
    Random8Core(const Random8Core &) = delete;
    Random8Core &operator=(const Random8Core &) = delete;
    Random8Core(Random8Core &&) noexcept;
    Random8Core &operator=(Random8Core &&) noexcept;
    ~Random8Core();

    void reset();
    void resetChannel(int channel);
    R8::Channel::ChannelState getChannelState(int channel) const;
    void setChannelState(int channel, const R8::Channel::ChannelState &state);
    void setChannelParams(int channel, const Random8ChannelParams &params);
    void onTrigger(int channel);
    void process(float deltaSeconds);
    float getOutputVolts(int channel) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
