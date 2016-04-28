#pragma once

#include <stdint.h>
#include <complex>
#include <vector>
#include <glm/glm.hpp>



namespace MUFDTD{
	// OpenGL Mathematicsを使用する
	using namespace glm;



#ifndef FFFDTD_DOUBLE_PRECISION_REAL
	// 実数型
	using real = float;

	// 実数型2次元ベクトル
	using rvec2 = fvec2;

	// 実数型3次元ベクトル
	using rvec3 = fvec3;

	// 実数型4次元ベクトル
	using rvec4 = fvec4;
#else
	// 実数型
	using real = double;

	// 実数型2次元ベクトル
	using rvec2 = dvec2;

	// 実数型3次元ベクトル
	using rvec3 = dvec3;

	// 実数型4次元ベクトル
	using rvec4 = dvec4;
#endif

	// 倍精度複素数型
	using complex = std::complex<double>;

	// 材質IDを保持する型
	using matid_t = uint16_t;

	// 係数インデックス型
	using cindex_t = uint16_t;

	// 係数インデックス型2次元ベクトル
	using cindex2_t = u16vec2;

	// 係数インデックス型3次元ベクトル
	using cindex3_t = u16vec3;

	// 係数インデックス型4次元ベクトル
	using cindex4_t = u16vec4;

	// 座標インデックス型
	using index_t = uint32_t;

	// 座標インデックス型2次元ベクトル
	using index2_t = u32vec2;

	// 座標インデックス型3次元ベクトル
	using index3_t = u32vec3;

	// 座標インデックス型4次元ベクトル
	using index4_t = u32vec4;

	// 符号付き座標インデックス型
	using sindex_t = int32_t;

	// 符号付き座標インデックス型2次元ベクトル
	using sindex2_t = i32vec2;

	// 符号付き座標インデックス型3次元ベクトル
	using sindex3_t = i32vec3;

	// 符号付き座標インデックス型4次元ベクトル
	using sindex4_t = i32vec4;
	
	

	// 向き
	enum DIR_e{
		X_PLUS = 1,
		Y_PLUS = 2,
		Z_PLUS = 3,
		X_MINUS = -X_PLUS,
		Y_MINUS = -Y_PLUS,
		Z_MINUS = -Z_PLUS
	};

	// 軸
	enum AXIS_e{
		AXIS_X = 1 << 0,
		AXIS_Y = 1 << 1,
		AXIS_Z = 1 << 2
	};

	// 面
	enum PLANE_e{
		X_PLANE = X_PLUS,	// YZ面
		Y_PLANE = Y_PLUS,	// XZ面
		Z_PLANE = Z_PLUS	// XY面
	};

	// 面のビットマスク
	enum PLANE_MASK_e{
		X_PLANE_MASK = 1 << X_PLANE,
		Y_PLANE_MASK = 1 << Y_PLANE,
		Z_PLANE_MASK = 1 << Z_PLANE,
		ALL_PLANE_MASK = X_PLANE_MASK | Y_PLANE_MASK | Z_PLANE_MASK
	};

	// 周波数特性の種類
	enum FrequencyResponce_e{
		FR_DFT,		// 離散フーリエ変換
		FR_ESD,		// エネルギースペクトル密度
		FR_PSD		// パワースペクトル密度
	};



	// 電磁界の周波数成分を格納する構造体
	struct EMFreq_t{
		complex E_x;
		complex E_y;
		complex E_z;
		complex H_x;
		complex H_y;
		complex H_z;
	};

	// 電磁界の瞬時値を格納する構造体
	struct EMTime_t{
		real E_x;
		real E_y;
		real E_z;
		real H_x;
		real H_y;
		real H_z;
	};

	
	
	
	
	
	
	
	
	// 境界条件の種類の定義
	enum class BoundaryCondition{
		PEC = 0,		// 電気壁
		PML,			// PML境界条件
		Periodic,		// 周期境界条件
	};

	
	


}
