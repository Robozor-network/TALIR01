#ifndef __POLY_H
#define __POLY_H

#include <vector>
#include <iostream>

#include <math.h>

class poly {
public:
	poly(std::vector<double> factors)
		: factors(factors)
	{}

	double operator() (double x) const {
		double acc = 0;
		int order = 0;
		for (auto i = factors.begin(); i != factors.end(); i++) {
			acc += *i * pow(x, order);
			order += 1;
		}
		return acc;
	}

	double operator[] (int i) const {
		if (i < 0 || i >= factors.size())
			return 0;
		else
			return factors[i];
	}

	int len() const {
		return factors.size();
	}

	poly operator+(const poly &other) const {
		std::vector<double> f;
		for (int i = 0; i < std::max(len(), other.len()); i++) {
			f.push_back((*this)[i] + other[i]);
		}
		return poly(f);
	}

	poly operator*(const poly &other) const {
		std::vector<double> f(len() + other.len() - 1);
		for (int i = 0; i < len(); i++)
			for (int j = 0; j < other.len(); j++)
				f[i + j] += (*this)[i]*other[j];
		return poly(f);
	}

	poly operator*(const double s) const {
		std::vector<double> f(len());
		for (int i = 0; i < len(); i++)
			f[i] = (*this)[i] * s;
		return poly(f);
	}

	poly operator/(const double s) const {
		std::vector<double> f(len());
		for (int i = 0; i < len(); i++)
			f[i] = (*this)[i] / s;
		return poly(f);
	}

	poly d() const {
		std::vector<double> f;
		for (int i = 1; i < len(); i++)
			f.push_back(i*(*this)[i]);
		return poly(f);
	}

	poly norm(double s) const {
		return (*this) / (*this)(s);
	}

    friend std::ostream& operator<<(std::ostream &os, const poly &p);

    static poly with_zeros(std::vector<double> zeros) {
    	poly p({1.0});
    	for (auto i = zeros.begin(); i != zeros.end(); i++)
    		p = p*poly({-*i, 1});
    	return p;
    }

private:
	std::vector<double> factors;
};

std::ostream &operator<<(std::ostream &os, const poly &p)
{
	os << "{";
	for (int i = 0; i < p.len(); i++) {
		if (i != 0) os << " ";
		os << p[p.len() - i - 1];
	}
	os << "}";
    return os;
}

poly cubic(double x1, double x2, double y1, double y2, double y1_, double y2_) {
	poly p = poly::with_zeros({x1}).norm(x2)*y2 \
			 + poly::with_zeros({x2}).norm(x1)*y1;

	{
		poly z = poly::with_zeros({x1, x1, x2});
		p = p + z/(z.d()(x2))*(y2_-p.d()(x2));
	}

	{
		poly z = poly::with_zeros({x2, x2, x1});
		p = p + z/(z.d()(x1))*(y1_-p.d()(x1));
	}

	return p;
}

poly quadratic(double x1, double x2, double y1, double y2, double y1_) {
	poly p = poly::with_zeros({x1}).norm(x2)*y2 \
			 + poly::with_zeros({x2}).norm(x1)*y1;

	{
		poly z = poly::with_zeros({x2, x1});
		p = p + z/(z.d()(x1))*(y1_-p.d()(x1));
	}

	return p;
}

#endif /* __POLY_H */
