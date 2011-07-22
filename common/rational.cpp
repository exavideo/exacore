/*
 * Copyright 2011 Exavideo LLC.
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

#include "rational.h"
#include <assert.h>
#include <stdio.h>

Rational::Rational(int number) {
    _num = number;
    _denom = 1;
}

Rational::Rational(int num, int denom) {
    _num = num;
    _denom = denom;

    if (_denom < 0) {
        throw RationalDivisionByZeroException( ); 
    }

    reduce( );
}

Rational::Rational(const Rational &r) {
    _num = r._num;
    _denom = r._denom;
}

int Rational::num( ) const {
    return _num;
}

int Rational::denom( ) const {
    return _denom;
}

int Rational::integer_part( ) const {
    if (_num > 0) {
        return _num / _denom;
    } else {
        /* avoid "implementation-defined" behavior */
        return -((-_num) / _denom);
    }
}

Rational Rational::fractional_part( ) const {
    if (_num > 0) {
        return Rational(_num % _denom, _denom);
    } else {
        /* work around potential C++ weirdness */
        return Rational(-((-_num) % _denom), _denom);
    }
}

Rational Rational::inverse( ) const {
    return Rational(_denom, _num);
};

const Rational &Rational::operator=(const Rational &rhs) {
    _num = rhs._num;
    _denom = rhs._denom;

    return *this;
}

Rational Rational::operator-( ) const {
    return Rational(-_num, _denom); 
}

const Rational &Rational::operator+=(const Rational &rhs) {
    int cd = lcm(_denom, rhs._denom);
    int m1 = cd / _denom;
    int m2 = cd / rhs._denom;

    _num = _num * m1 + rhs._num * m2;
    _denom = cd;
    reduce( );

    return *this;
}

const Rational &Rational::operator-=(const Rational &rhs) {
    *this += -rhs;
    return *this;
}

const Rational &Rational::operator*=(const Rational &rhs) {
    _num *= rhs.num( );
    _denom *= rhs.denom( );
    reduce( );

    return *this;
}

const Rational &Rational::operator/=(const Rational &rhs) {
    *this *= rhs.inverse( );
    return *this;
}

Rational Rational::operator+(const Rational &rhs) const {
    Rational result(*this);
    result += rhs;
    return result;
}

Rational Rational::operator-(const Rational &rhs) const {
    Rational result(*this);
    result -= rhs;
    return result;
}

Rational Rational::operator*(const Rational &rhs) const {
    Rational result(*this);
    result *= rhs;
    return result;
}

Rational Rational::operator/(const Rational &rhs) const {
    Rational result(*this);
    result /= rhs;
    return result;
}

bool Rational::less_than_one_half( ) const {
    if (2 * _num < _denom) {
        fprintf(stderr, "%d/%d < 1/2\n", _num, _denom);
        return true;
    } else {
        fprintf(stderr, "%d/%d >= 1/2\n", _num, _denom);
        return false;
    }
}

int Rational::gcd(int a, int b) {
    int gcd;

    assert(a != 0 || b != 0);

    if (a < 0) { a = -a; }
    if (b < 0) { b = -b; }

    while (a != 0 && b != 0) {
        if (a > b) {
            a = a % b; 
        } else {
            b = b % a;
        }
    }

    gcd = (a != 0) ? a : b;

    assert(gcd != 0);
    assert(a % gcd == 0);
    assert(b % gcd == 0);

    return gcd;
}

int Rational::lcm(int a, int b) {
    assert(a > 0);
    assert(b > 0);

    return a * b / gcd(a, b);    
}

void Rational::reduce(void) {
    assert(_denom != 0);

    /* move any negative signs to the numerator */
    if (_denom < 0) {
        _num = -_num;
        _denom = -_denom;
    }

    int gc = gcd(_num, _denom);
    _num /= gc;
    _denom /= gc;
}
