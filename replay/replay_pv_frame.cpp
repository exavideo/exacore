/*
 * Copyright 2011, 2012, 2013 Exavideo LLC.
 * 
 * This file is part of openreplay.
 * 
 * openreplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * openreplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with openreplay.  If not, see <http://www.gnu.org/licenses/>.
 */

ReplayPvFrame::ReplayPvFrame( ) {
    data = NULL;
    size = 0;
}

ReplayPvFrame::ReplayPvFrame(DeserializeStream &str) {
    data = NULL;
    size = 0;
    deserialize(str);
}

void ReplayPvFrame::serialize(SerializeStream &str) {
    str << size;
    str << format;
    str.write_array_byref(data, size);
}

void ReplayPvFrame::deserialize(DeserializeStream &str) {
    str >> size;
    str >> format;
    str.read_array(data, size);
}

void ReplayPvFrame::set_fft(float *data, size_t fft_size) {
    if (size == 0) {
        this->src_data = new float[fft_size];
    } else if (size != fft_size) {
        throw std::runtime_error("cannot change size of ReplayPvFrame");
    }
}

void ReplayPvFrame::make_polar( ) {    
    float re, im, mag, phase;
    float *rp, *ip;

    if (format == POLAR) {
        return;
    }

    rp = &data[1];
    ip = &data[size-2];
    for (i = 1; i < size/2; i++) {
        re = *rp;
        im = *ip;
        mag = sqrtf(re*re + im*im);
        phase = atan2f(im, re);
        *rp = mag;
        *ip = phase;
        rp++;
        ip--;
    }

    format = POLAR;
}

void ReplayPvFrame::make_rectangular( ) {
    float mag, phase, re, im;
    float *rp, ip;

    if (format == RECTANGULAR) {
        return;
    }

    rp = &data[1];
    ip = &data[size-2];
    for (i = 1; i < size/2; i++) {
        mag = *rp;
        phase = *ip;
        re = mag * cosf(phase);
        im = mag * sinf(phase);
        *rp = re;
        *im = im;
        rp++;
        im--;
    }

    format = RECTANGULAR;
}
