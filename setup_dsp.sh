#!/bin/bash

# Create DSP library directory
mkdir -p lib/dsp

# Create DSP filter header files
cat > lib/dsp/FineTuner.h << 'EOL'
#pragma once
#include <vector>
#include <complex>

class FineTuner {
public:
    FineTuner(unsigned int sampleRate, int freqOffset);
    void process(const std::vector<std::complex<float>>& in, std::vector<std::complex<float>>& out);
private:
    unsigned int _sampleRate;
    int _freqOffset;
    float _phase;
};
EOL

cat > lib/dsp/LowPassFilterFirIQ.h << 'EOL'
#pragma once
#include <vector>
#include <complex>

class LowPassFilterFirIQ {
public:
    LowPassFilterFirIQ(unsigned int sampleRate, double cutoff);
    void process(const std::vector<std::complex<float>>& in, std::vector<std::complex<float>>& out);
private:
    std::vector<float> _taps;
    std::vector<std::complex<float>> _buffer;
};
EOL

cat > lib/dsp/DownsampleFilter.h << 'EOL'
#pragma once
#include <vector>

class DownsampleFilter {
public:
    DownsampleFilter(unsigned int sampleRate, double cutoff, double factor, bool useHanning);
    void process(const std::vector<double>& in, std::vector<double>& out);
private:
    std::vector<float> _taps;
    std::vector<double> _buffer;
    unsigned int _factor;
};
EOL

cat > lib/dsp/HighPassFilterIir.h << 'EOL'
#pragma once
#include <vector>

class HighPassFilterIir {
public:
    HighPassFilterIir(double cutoff);
    void process_inplace(std::vector<double>& data);
private:
    double _alpha;
    double _prev;
};
EOL

cat > lib/dsp/LowPassFilterRC.h << 'EOL'
#pragma once
#include <vector>

class LowPassFilterRC {
public:
    LowPassFilterRC(double timeConstant);
    void process_inplace(std::vector<double>& data);
private:
    double _alpha;
    double _prev;
};
EOL

# Create DSP filter implementation files
cat > lib/dsp/FineTuner.cpp << 'EOL'
#include "FineTuner.h"
#include <cmath>

FineTuner::FineTuner(unsigned int sampleRate, int freqOffset)
    : _sampleRate(sampleRate), _freqOffset(freqOffset), _phase(0) {}

void FineTuner::process(const std::vector<std::complex<float>>& in, std::vector<std::complex<float>>& out) {
    out.resize(in.size());
    float phaseIncrement = 2.0f * M_PI * _freqOffset / _sampleRate;
    
    for (size_t i = 0; i < in.size(); ++i) {
        float cosVal = cos(_phase);
        float sinVal = sin(_phase);
        out[i] = std::complex<float>(
            in[i].real() * cosVal - in[i].imag() * sinVal,
            in[i].real() * sinVal + in[i].imag() * cosVal
        );
        _phase += phaseIncrement;
        if (_phase > 2.0f * M_PI) _phase -= 2.0f * M_PI;
    }
}
EOL

cat > lib/dsp/LowPassFilterFirIQ.cpp << 'EOL'
#include "LowPassFilterFirIQ.h"
#include <cmath>

LowPassFilterFirIQ::LowPassFilterFirIQ(unsigned int sampleRate, double cutoff) {
    int numTaps = 101;
    _taps.resize(numTaps);
    double fc = cutoff / sampleRate;
    
    for (int i = 0; i < numTaps; i++) {
        if (i == numTaps/2) {
            _taps[i] = 2.0f * fc;
        } else {
            double x = M_PI * (i - numTaps/2);
            _taps[i] = sin(2.0f * fc * x) / x;
        }
        _taps[i] *= 0.42f - 0.5f * cos(2.0f * M_PI * i / (numTaps-1))
                    + 0.08f * cos(4.0f * M_PI * i / (numTaps-1));
    }
    _buffer.resize(numTaps);
}

void LowPassFilterFirIQ::process(const std::vector<std::complex<float>>& in, std::vector<std::complex<float>>& out) {
    out.resize(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        _buffer.push_back(in[i]);
        _buffer.erase(_buffer.begin());
        std::complex<float> sum = 0;
        for (size_t j = 0; j < _taps.size(); ++j) {
            sum += _buffer[j] * _taps[j];
        }
        out[i] = sum;
    }
}
EOL

cat > lib/dsp/DownsampleFilter.cpp << 'EOL'
#include "DownsampleFilter.h"
#include <cmath>

DownsampleFilter::DownsampleFilter(unsigned int sampleRate, double cutoff, double factor, bool useHanning)
    : _factor(factor) {
    int numTaps = 101;
    _taps.resize(numTaps);
    double fc = cutoff / sampleRate;
    
    for (int i = 0; i < numTaps; i++) {
        if (i == numTaps/2) {
            _taps[i] = 2.0f * fc;
        } else {
            double x = M_PI * (i - numTaps/2);
            _taps[i] = sin(2.0f * fc * x) / x;
        }
        if (useHanning) {
            _taps[i] *= 0.5f * (1.0f - cos(2.0f * M_PI * i / (numTaps-1)));
        }
    }
    _buffer.resize(numTaps);
}

void DownsampleFilter::process(const std::vector<double>& in, std::vector<double>& out) {
    out.resize(in.size() / _factor);
    for (size_t i = 0; i < in.size(); i += _factor) {
        _buffer.push_back(in[i]);
        _buffer.erase(_buffer.begin());
        if (i % _factor == 0) {
            double sum = 0;
            for (size_t j = 0; j < _taps.size(); ++j) {
                sum += _buffer[j] * _taps[j];
            }
            out[i/_factor] = sum;
        }
    }
}
EOL

cat > lib/dsp/HighPassFilterIir.cpp << 'EOL'
#include "HighPassFilterIir.h"
#include <cmath>

HighPassFilterIir::HighPassFilterIir(double cutoff)
    : _alpha(1.0 / (1.0 + 2.0 * M_PI * cutoff)), _prev(0) {}

void HighPassFilterIir::process_inplace(std::vector<double>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        double tmp = data[i];
        data[i] = _alpha * (data[i] - _prev);
        _prev = tmp;
    }
}
EOL

cat > lib/dsp/LowPassFilterRC.cpp << 'EOL'
#include "LowPassFilterRC.h"
#include <cmath>

LowPassFilterRC::LowPassFilterRC(double timeConstant)
    : _alpha(1.0 - exp(-1.0/timeConstant)), _prev(0) {}

void LowPassFilterRC::process_inplace(std::vector<double>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = _prev + _alpha * (data[i] - _prev);
        _prev = data[i];
    }
}
EOL

echo "DSP filter files created successfully!"
