#include "FFWaveform.h"
#include <exprtk.hpp>



namespace FFFDTD{
	// 数式パーサー関連のメンバーを格納する構造体
	struct FFWaveform::expr_t{
		// 時刻t
		double t;

		// シンボルテーブル
		exprtk::symbol_table<double> symbol_table;

		// 数式
		exprtk::expression<double> expression;

		// パーサー
		exprtk::parser<double> parser;
	};

	// コンストラクタ
	FFWaveform::FFWaveform(const std::string &expr_string){
		m_Expr = new expr_t;

		// シンボルテーブルを作成
		m_Expr->symbol_table.add_variable("t", m_Expr->t);
		m_Expr->symbol_table.add_constants();

		// シンボルテーブルをセットする
		m_Expr->expression.register_symbol_table(m_Expr->symbol_table);

		// 数式をコンパイルする
		if (m_Expr->parser.compile(expr_string, m_Expr->expression) == false){
			throw;
		}
	}

	// デストラクタ
	FFWaveform::~FFWaveform(){
		delete m_Expr;
	}

	// 時刻tでの値を計算する
	double FFWaveform::getValue(double t){
		m_Expr->t = t;
		return m_Expr->expression.value();
	}
}
