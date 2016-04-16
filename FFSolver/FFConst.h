#pragma once

#include "FFType.h"



namespace MUFDTD{
	// 円周率
	static const double PI = 3.14159265358979323846;

	// 真空中の光速
	static const double C = 2.99792458e+8;

	// 自由空間の誘電率
	static const double EPS_0 = 8.854187817e-12;

	// 自由空間の透磁率
	static const double MU_0 = 1.256637061e-6;

	// ネイピア数
	static const double E = 2.718281828459045;

	// 扱える領域の一辺の最大サイズ
	static const index_t MAX_SIZE = 65535;
}
