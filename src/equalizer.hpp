#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <omp.h>
#include <qbuffer.h>
#include <sndfile.h>

#include <QBuffer>

#include "type_aliases.hpp"

constexpr u8 BANDS = 10;
using namespace juce::dsp::IIR;

auto readAudioFile(const path& inputFile) -> tuple<SF_INFO, vector<f32>> {
    SF_INFO sfInfo;
    SNDFILE* inFile = sf_open(inputFile.string().c_str(), SFM_READ, &sfInfo);

    if (inFile == nullptr) {
        std::cerr << "Error opening input file: " << sf_strerror(nullptr)
                  << '\n';
        return {};
    }

    i32 channels = sfInfo.channels;
    i64 numFrames = sfInfo.frames;

    vector<f32> audioData(numFrames * channels);
    sf_readf_float(inFile, audioData.data(), numFrames);
    sf_close(inFile);

    return {sfInfo, audioData};
}

auto outputAudioFile(SF_INFO info, const path& outputFile,
                     const vector<f32>& data) -> bool {
    SNDFILE* outFile = sf_open(outputFile.string().c_str(), SFM_WRITE, &info);

    if (outFile == nullptr) {
        std::cerr << "Error opening output file." << '\n';
        return false;
    }

    sf_write_float(outFile, data.data(), static_cast<sf_count_t>(data.size()));
    sf_close(outFile);
    return true;
}

auto makeAudioBuffer(const vector<f32>& audioData) -> QBuffer* {
    auto* qBuffer = new QBuffer();
    qBuffer->setData(reinterpret_cast<const char*>(audioData.data()),
                     static_cast<i32>(audioData.size() * sizeof(f32)));
    qBuffer->open(QIODevice::ReadOnly);
    return qBuffer;
}

auto equalizeAudioFile(const path& inputFile, const path& outputFile,
                       const array<f32, BANDS>& gains)
    -> tuple<i32, i32, QBuffer*> {
    auto [sfInfo, audioData] = readAudioFile(inputFile);
    if (audioData.empty()) {
        return {};
    }

    u32 sampleRate = sfInfo.samplerate;
    i32 channels = sfInfo.channels;
    i64 numFrames = sfInfo.frames;

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

    juce::AudioBuffer<f32> buffer(channels, static_cast<int>(numFrames));

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        memcpy(buffer.getWritePointer(ch), &audioData[ch * numFrames],
               numFrames * sizeof(f32));
    }

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        f32* ptr = buffer.getWritePointer(ch);
        f32* const* ref = &ptr;

        juce::dsp::AudioBlock<f32> channelBlock(ref, 1, numFrames);
        juce::dsp::ProcessContextReplacing<f32> context(channelBlock);

        for (u8 band = 0; band < BANDS; band++) {
            filters[ch][band].process(context);
        }
    }

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        memcpy(&audioData[ch * numFrames], buffer.getReadPointer(ch),
               numFrames * sizeof(f32));
    }

    outputAudioFile(sfInfo, outputFile, audioData);
    return {sampleRate, static_cast<u32>(channels), makeAudioBuffer(audioData)};
}
