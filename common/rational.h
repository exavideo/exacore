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

#ifndef _RATIONAL_H
#define _RATIONAL_H

#include <stdexcept>

class RationalDivisionByZeroException : public virtual std::exception {
    const char *what() const throw() { return "Illegal rational operation"; }
};

/*
 * Note: Overflow of rational operations is subject to undefined behavior.
 */
class Rational {
    public:
        Rational( );
        Rational(int number);
        Rational(int num, int denom);
        Rational(const Rational& r);
        
        int num( ) const;
        int denom( ) const;
        float to_float( ) const;

        int integer_part( ) const;
        Rational fractional_part( ) const;
        Rational inverse( ) const;

        const Rational &operator=(const Rational &rhs);

        Rational operator-( ) const;

        const Rational &operator+=(const Rational &rhs);
        const Rational &operator-=(const Rational &rhs);
        const Rational &operator*=(const Rational &rhs);
        const Rational &operator/=(const Rational &rhs);
        
        Rational operator+(const Rational &rhs) const;
        Rational operator-(const Rational &rhs) const;
        Rational operator*(const Rational &rhs) const;
        Rational operator/(const Rational &rhs) const;

        bool operator<(const Rational &rhs) const;
        bool operator<=(const Rational &rhs) const;
        bool operator==(const Rational &rhs) const;
        bool operator>=(const Rational &rhs) const;
        bool operator>(const Rational &rhs) const;

        /* fast shortcut to check if a rational is less than one half */
        bool less_than_one_half( ) const;

    protected:
        /* Always call reduce( ) after an update */
        int _num;
        int _denom;
        
        void reduce( );
        static int lcm(int a, int b);
        static int gcd(int a, int b);
};

#endif
