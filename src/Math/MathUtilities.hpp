#pragma once
template<class T>
struct Fract { // p/q
	T p = 0;
	T q = 1;
	Fract(T _p, T _q) : p(_p), q(_q) {}
	Fract(T _p) : p(_p), q(1) {}
	Fract() : p(0), q(1) {}
	Fract operator + (Fract a) {
		return Fract(p * a.q + q * a.p, q * a.q);
	}
	Fract operator - (Fract a) {
		return Fract(p * a.q - q * a.p, q * a.q);
	}
	Fract operator * (Fract a) {
		return Fract(p * a.p, q * a.q);
	}
	Fract operator / (Fract a) {
		return Fract(p * a.q, q * a.p);
	}
};

using lfract = Fract<long long>;