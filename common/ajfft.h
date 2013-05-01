/*
 * Andrew's Janky FFT v0.1
 * Copyright (C) 2013 Andrew H. Armenia
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this 
 * list of conditions and the following disclaimer.  Redistributions in binary 
 * form must reproduce the above copyright notice, this list of conditions and 
 * the following disclaimer in the documentation and/or other materials 
 * provided with the distribution. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT 
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AJFFT_H
#define AJFFT_H

#include <complex>
#include <stddef.h>

template <typename T>
class FFT {
	public:
		enum direction { FORWARD, INVERSE };
		FFT(size_t sz, direction dir = FORWARD);
		~FFT( );
		
		void compute(std::complex<T> *dst, const T *src);
		void compute(std::complex<T> *dst, const std::complex<T> *src);
	private:
		/* most FFT descriptions call this W_n(k) */
		std::complex<T> *twiddles;
		/* 
		 * implement the input odd-even permutation 
		 * e.g. bitrev[0x55] = 0xaa for size=256
		 */
		size_t *bitrev;

		/* the FFT size (and half of it) */
		size_t size;
		size_t halfsize;

		void bfly(std::complex<T> &u, std::complex<T> &b, size_t t);
		void work(std::complex<T> *a, size_t n, size_t s);
		void copy_in(std::complex<T> *dst, const T *src);
		void copy_in(std::complex<T> *dst, const std::complex<T> *src);
};

template <typename T>
FFT<T>::FFT(size_t sz, direction dir) {
	/* obtain pi in the type we are using */
	const T pi = std::arg(std::complex<T>(T(-1), T(0)));

	T angle;

	/* check if size is a power of 2 FIXME */

	size = sz;
	halfsize = sz / 2;

	bitrev = new size_t[sz];
	twiddles = new std::complex<T>[sz];

	/* compute all twiddles */
	for (size_t i = 0; i < sz; i++) {
		if (dir == FORWARD) {
			angle = -(T(2) * pi * T(i) / T(sz));
		} else {
			angle = T(2) * pi * T(i) / T(sz);
		}
		twiddles[i] = std::polar<T>(T(1), angle);
	}

	/* 
	 * compute bit-reversed permutation table.
	 * This is an adaptation of a code snippet found at
	 * http://www.musicdsp.org/showone.php?id=171
	 * not sure I want to know why this works...
	 */
	size_t r = 0;
	size_t s = 0;
	size_t i = 0;
	while (i < sz) {
		bitrev[i] = s;
		r += 2;
		i += 1;
		s ^= sz - (sz / (r & -r));
	}
}

template <typename T>
FFT<T>::~FFT( ) {
	delete [] bitrev;
	delete [] twiddles;
}

/* 
 * This is the basic calculation of the FFT.
 * u = u + b*twiddles[t], b = u + b * twiddles[u+halfsize] 
 */
template <typename T>
void FFT<T>::bfly(std::complex<T> &u, std::complex<T> &b, size_t t) {
	/* compute the twiddled component of b to add to u */
	std::complex<T> tmp = b * twiddles[t];

	/* twiddle b and add u */
	b *= twiddles[t + halfsize];
	b += u;

	/* add previously twiddled b to u */
	u += tmp;
}

template <typename T>
void FFT<T>::work(std::complex<T> *a, size_t n, size_t s) {
	/* 
	 * a is the work array, 
	 * n is the current dft size, 
	 * s is the stride for accessing the twiddles.
	 */
	size_t k;
	if (n == 2) {
		/* 2-point DFT is just a single butterfly */
		bfly(a[0], a[1], 0);
	} else {
		/* divide into subtransforms, then merge */
		work(a, n/2, s*2);
		work(a+n/2, n/2, s*2);
		for (k = 0; k < n/2; k++) {
			bfly(a[k], a[k + n/2], s * k);
		}
	}
}

template <typename T>
void FFT<T>::copy_in(std::complex<T> *dst, const T *src) {
	for (size_t i = 0; i < size; i++) {
		dst[bitrev[i]] = src[i];
	}
}

template <typename T>
void FFT<T>::copy_in(std::complex<T> *dst, const std::complex<T> *src) {
	for (size_t i = 0; i < size; i++) {
		dst[bitrev[i]] = src[i];
	}
}

template <typename T>
void FFT<T>::compute(std::complex<T> *dst, const T *src) {
	copy_in(dst, src);
	work(dst, size, 1);
}

template <typename T>
void FFT<T>::compute(std::complex<T> *dst, const std::complex<T> *src) {
	copy_in(dst, src);
	work(dst, size, 1);
}

#endif
