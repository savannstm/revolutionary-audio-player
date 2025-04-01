#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <omp.h>
#include <qbuffer.h>
#include <sndfile.h>

#include <QBuffer>
#include <format>

#include "type_aliases.hpp"

constexpr u8 BANDS = 10;
using namespace juce::dsp::IIR;

inline auto getDuration(const u16 chn, const u32 sampleRate,
                        const vector<f32>& data) -> u32 {
    return data.size() / (static_cast<u32>(chn * sampleRate));
}

inline auto readAudioFile(const path& inputFile)
    -> tuple<SF_INFO, u32, vector<f32>> {
    SF_INFO sfInfo;
    SNDFILE* inFile = sf_open(inputFile.string().c_str(), SFM_READ, &sfInfo);

    if (inFile == nullptr) {
        println(cout, "Error opening input file: {}", sf_strerror(inFile));
        return {};
    }

    const u32 sampleRate = sfInfo.samplerate;
    const u16 channels = sfInfo.channels;
    const i64 frames = sfInfo.frames;

    vector<f32> audioData(frames * channels);
    sf_readf_float(inFile, audioData.data(), frames);
    sf_close(inFile);

    const u32 duration = getDuration(channels, sampleRate, audioData);

    return {sfInfo, duration, audioData};
}

inline auto outputAudioFile(SF_INFO info, const path& outputFile,
                            const vector<f32>& data) -> bool {
    SNDFILE* outFile = sf_open(outputFile.string().c_str(), SFM_WRITE, &info);

    if (outFile == nullptr) {
        println(cerr, "Error opening output file. {}", sf_strerror(outFile));
        return false;
    }

    sf_write_float(outFile, data.data(), static_cast<sf_count_t>(data.size()));
    sf_close(outFile);
    return true;
}

inline auto makeAudioBuffer(const vector<f32>& audioData) -> QBuffer* {
    auto* qBuffer = new QBuffer();
    qBuffer->setData(reinterpret_cast<const char*>(audioData.data()),
                     static_cast<i32>(audioData.size() * sizeof(f32)));
    qBuffer->open(QIODevice::ReadOnly);
    return qBuffer;
}

inline auto equalizeAudioFile(const path& inputFile, const path& outputFile,
                              const array<f32, BANDS>& gains)
    -> tuple<SF_INFO, u32, QBuffer*> {
    auto [sfInfo, _duration, audioData] = readAudioFile(inputFile);
    if (audioData.empty()) {
        return {};
    }

    const u32 sampleRate = sfInfo.samplerate;
    const u16 channels = sfInfo.channels;
    const i64 frames = sfInfo.frames;

    constexpr array<f32, BANDS> centerFrequencies = {
        31.0,   62.0,   125.0,  250.0,  500.0,
        1000.0, 2000.0, 4000.0, 8000.0, 16000.0};
    constexpr f32 QFactor = 1.0;

    vector<array<Filter<f32>, BANDS>> filters(channels);

#pragma omp parallel for collapse(2)
    for (i32 ch = 0; ch < channels; ch++) {
        for (u8 band = 0; band < BANDS; band++) {
            filters[ch][band].coefficients = Coefficients<f32>::makePeakFilter(
                sampleRate, centerFrequencies[band], QFactor, gains[band]);
            filters[ch][band].reset();
        }
    }

    juce::AudioBuffer<f32> buffer(channels, static_cast<i32>(frames));

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        memcpy(buffer.getWritePointer(ch), &audioData[ch * frames],
               frames * sizeof(f32));
    }

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        f32* ptr = buffer.getWritePointer(ch);
        f32* const* ref = &ptr;

        juce::dsp::AudioBlock<f32> channelBlock(ref, 1, frames);
        juce::dsp::ProcessContextReplacing<f32> context(channelBlock);

        for (u8 band = 0; band < BANDS; band++) {
            filters[ch][band].process(context);
        }
    }

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        memcpy(&audioData[ch * frames], buffer.getReadPointer(ch),
               frames * sizeof(f32));
    }

    const u32 duration = getDuration(channels, sampleRate, audioData);
    return {sfInfo, duration, makeAudioBuffer(audioData)};
}

inline auto getAudioFile(const path& inputFile)
    -> tuple<SF_INFO, u32, QBuffer*> {
    auto [sfInfo, duration, audioData] = readAudioFile(inputFile);
    return {sfInfo, duration, makeAudioBuffer(audioData)};
}