#pragma once

#include "FFType.h"



namespace FFFDTD{
	// 電圧波形を生成するクラス
	class FFWaveform{
	private:
		// 数式パーサー関連のメンバーを格納する構造体
		struct expr_t;

		// 数式パーサー関連のメンバー
		expr_t *m_Expr;

	public:
		// コンストラクタ
		FFWaveform(const std::string &expr_string);

		// デストラクタ
		~FFWaveform();

		// 時刻tでの値を計算する
		double getValue(double t);
	};
}
