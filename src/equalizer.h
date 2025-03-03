#include <qlogging.h>
#include <sndfile.h>

#include "iir.h"

// Nothing fucking works and I don't like it
void process_audio(const char *input_path, const char *output_path,
                   const float eq_gains[10]) {
    SF_INFO sfinfo;
    SNDFILE *infile;
    SNDFILE *outfile;

    infile = sf_open(input_path, SFM_READ, &sfinfo);
    if (infile == nullptr) {
        return;
    }

    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    outfile = sf_open(output_path, SFM_WRITE, &sfinfo);
    if (outfile == nullptr) {
        sf_close(infile);
        return;
    }

    const auto sampling_rate = sfinfo.samplerate;

    const int numBands = 10;
    const int order = 4;

    Iir::Butterworth::BandPass<order> filters[numBands];

    float centerFrequencies[numBands] = {31.0f,   62.0f,   125.0f,  250.0f,
                                         500.0f,  1000.0f, 2000.0f, 4000.0f,
                                         8000.0f, 16000.0f};

    const float Q = 1.0f;

    for (int i = 0; i < numBands; i++) {
        filters[i].setup(sampling_rate, centerFrequencies[i], Q);
    }

    const int BUFFER_LEN = 1024;
    float buffer[BUFFER_LEN];
    sf_count_t read_count;

    while ((read_count = sf_read_float(infile, buffer, BUFFER_LEN)) > 0) {
        for (int i = 0; i < read_count; i++) {
            float sample = buffer[i];
            float eq_sample = 0.0f;

            for (int j = 0; j < numBands; j++) {
                eq_sample += eq_gains[j] * filters[j].filter(sample);
            }

            buffer[i] = eq_sample;
        }
        sf_write_float(outfile, buffer, read_count);
    }

    sf_close(outfile);
    sf_close(infile);
}
