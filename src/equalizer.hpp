#include <juce_dsp/juce_dsp.h>
#include <sndfile.h>

#include "type_aliases.hpp"
constexpr u8 BANDS = 10;

using namespace juce::dsp::IIR;

void equalizeAudioFile(const std::string& inputFile,
                       const std::string& outputFile,
                       const std::array<f32, BANDS>& gains) {
    SF_INFO sfInfo;
    auto* inFile = sf_open(inputFile.c_str(), SFM_READ, &sfInfo);
    if (inFile == nullptr) {
        std::cerr << "Error opening input file: " << sf_strerror(inFile)
                  << '\n';
        return;
    }

    const u32 sampleRate = sfInfo.samplerate;
    const u32 channels = sfInfo.channels;
    const u32 numFrames = sfInfo.frames;

    std::vector<f32> audioData(numFrames * channels);
    sf_readf_float(inFile, audioData.data(), numFrames);
    sf_close(inFile);

    constexpr std::array<f32, BANDS> centerFrequencies = {
        31.0,   62.0,   125.0,  250.0,  500.0,
        1000.0, 2000.0, 4000.0, 8000.0, 16000.0};

    constexpr f32 QFactor = 1.0;

    std::vector<std::array<Filter<f32>, BANDS>> filters(channels);

    for (u32 ch = 0; ch < channels; ch++) {
        for (u8 band = 0; band < BANDS; band++) {
            const auto coeffs = Coefficients<f32>::makePeakFilter(
                sampleRate, centerFrequencies[band], QFactor, gains[band]);

            *filters[ch][band].coefficients = *coeffs;
            filters[ch][band].reset();
        }
    }

    for (sf_count_t frame = 0; frame < numFrames; frame++) {
        const u32 mul = (frame * channels);

        for (u32 ch = 0; ch < channels; ch++) {
            const u32 idx = mul + ch;
            f32 sample = audioData[idx];

            for (u32 band = 0; band < BANDS; band++) {
                sample = filters[ch][band].processSample(sample);
            }

            audioData[idx] = sample;
        }
    }

    auto outSfInfo = sfInfo;  // Inherit all info
    auto* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &outSfInfo);

    // TODO: Inherit bitrate mode from input file
    constexpr u32 bitrateMode = SF_BITRATE_MODE_CONSTANT;
    sf_command(outFile, SFC_SET_BITRATE_MODE, (void*)&bitrateMode,
               sizeof(bitrateMode));

    // Lower means better, for MPEG/Vorbis/FLAC
    constexpr f64 compression = 0.0;
    sf_command(outFile, SFC_SET_COMPRESSION_LEVEL, (void*)&compression,
               sizeof(compression));

    if (outFile == nullptr) {
        std::cerr << "Error opening output file: " << sf_strerror(outFile)
                  << '\n';
        return;
    }

    sf_writef_float(outFile, audioData.data(), numFrames);
    sf_close(outFile);
}
