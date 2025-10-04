#pragma once

#include <stdexcept>

namespace NumberLib {
	class Number {
	private:
		double value;
	public:
		Number(double val = 0.0) : value(val) {}

		Number operator*(const Number& other) const {
			return Number(this->value * other.value);
		}
		Number operator+(const Number& other) const {
			return Number(this->value + other.value);
		}
		Number operator-(const Number& other) const {
			return Number(this->value - other.value);
		}
		Number operator/(const Number& other) const {
			if (other.value == 0) {
				throw std::runtime_error("Не дели на 0");
			}
			return Number(this->value / other.value);
		}
		double getValue() const {
			return value;
		}
	};
	static const Number ZERO(0.0);
	static const Number ONE(1.0);
	static const Number CreateNumber(double val) {
		return Number(val);
	}
}