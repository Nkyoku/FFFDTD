#pragma once

#include "FFType.h"



namespace FFFDTD{
	// 一つの軸のグリッド情報を管理するクラス
	class FFGrid{
		/*** メンバー変数 ***/
	private:
		// グリッド間の間隔
		std::vector<double> m_Width;

		// グリッド上の間隔
		std::vector<double> m_MidWidth;

		// グリッドの位置
		std::vector<double> m_Pos;



		/*** メソッド ***/
	public:
		// コンストラクタ
		FFGrid(void);

		// コンストラクタ
		// 指定されたグリッドの一部を切り出す
		FFGrid(const FFGrid &grid, index_t offset, index_t size);

		// コンストラクタ
		// セル幅のベクターからグリッドを作成する
		FFGrid(const std::vector<double> &width);

		// グリッドの付随情報を計算する
		void precompute(bool periodic);

		// グリッドの分割数を取得する
		index_t count(void) const{
			return (index_t)m_Width.size();
		}

		// グリッド全体の最小間隔を取得する
		double minimumWidth(void) const;

		// グリッド間の間隔を取得する
		double width(index_t i) const{
			return m_Width[i];
		}

		// グリッド間の間隔の逆数を取得する
		double iwidth(index_t i) const{
			return 1.0 / m_Width[i];
		}

		// グリッド上の間隔を取得する
		double mwidth(index_t i) const{
			return m_MidWidth[i];
		}

		// グリッド上の間隔の逆数を取得する
		double imwidth(index_t i) const{
			return 1.0 / m_MidWidth[i];
		}

		// グリッドの位置を取得する
		double pos(index_t i) const{
			return m_Pos[i];
		}

		// 物理座標から論理座標を取得する
		double physicalToLogical(double ppos, bool *out_of_range = nullptr);

		// 論理座標から物理座標を取得する
		double logicalToPhysical(double lpos, bool *out_of_range = nullptr);
	};
}
