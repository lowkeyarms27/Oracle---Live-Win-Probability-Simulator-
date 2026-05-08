#include "audio.hpp"
#include <cmath>
#include <cstring>

static constexpr int SAMPLE_RATE = 44100;
static constexpr float FREQUENCY = 440.0f;
static int sampleIndex = 0;

void Audio::audioCallback(void* /*userdata*/, uint8_t* stream, int len) {
    auto* buf = reinterpret_cast<int16_t*>(stream);
    int samples = len / 2;
    for (int i = 0; i < samples; ++i) {
        buf[i] = static_cast<int16_t>(
            28000 * std::sin(2.0f * M_PI * FREQUENCY * sampleIndex++ / SAMPLE_RATE)
        );
    }
}

bool Audio::init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;

    SDL_AudioSpec want{}, got{};
    want.freq     = SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = 512;
    want.callback = audioCallback;

    deviceId = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
    return deviceId != 0;
}

void Audio::beep(bool on) {
    if (on == playing) return;
    SDL_PauseAudioDevice(deviceId, on ? 0 : 1);
    playing = on;
}

void Audio::destroy() {
    if (deviceId) SDL_CloseAudioDevice(deviceId);
    deviceId = 0;
}

Audio::~Audio() { destroy(); }
