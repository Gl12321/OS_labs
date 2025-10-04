#pragma once

#include "Number.h"
#include <cmath>

#ifdef _WIN32
#ifdef VECTORLIB_EXPORTS
#define VECTORLIB_API __declspec(dllexport)
#else
#define VECTORLIB_API __declspec(dllimport)
#endif
#else
#define VECTORLIB_API
#endif

namespace VectorLib {
    using namespace NumberLib;

    class Vector {
    private:
        Number x;
        Number y;
    public:
        VECTORLIB_API Vector(const Number& xVal, const Number& yVal);

        VECTORLIB_API Number getX() const;
        VECTORLIB_API Number getY() const;
        VECTORLIB_API Number getRadius() const;
        VECTORLIB_API Number getAngle() const;
        VECTORLIB_API Vector add(const Vector& other) const;
    };

    extern VECTORLIB_API const Vector ZERO_VECTOR;
    extern VECTORLIB_API const Vector ONE_VECTOR;

    Vector::Vector(const Number& xVal, const Number& yVal)
        : x(xVal), y(yVal) {}

    Number Vector::getX() const {
        return x;
    }

    Number Vector::getY() const {
        return y;
    }

    Number Vector::getRadius() const {
        return CreateNumber(std::sqrt(x.getValue() * x.getValue() + y.getValue() * y.getValue()));
    }

    Number Vector::getAngle() const {
        return CreateNumber(std::atan2(y.getValue(), x.getValue()));
    }

    Vector Vector::add(const Vector& other) const {
        return Vector(x + other.x, y + other.y);
    }

    const Vector ZERO_VECTOR(ZERO, ZERO);
    const Vector ONE_VECTOR(ONE, ONE);
}