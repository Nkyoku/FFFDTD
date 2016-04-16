#include "FFGrid.h"
#include <algorithm>



namespace MUFDTD{
	// コンストラクタ
	FFGrid::FFGrid(const FFGrid &grid, index_t offset, index_t size){
		m_Width.resize(size);
		for (index_t i = 0; i < size; i++){
			m_Width[i] = grid.m_Width[offset + i];
		}
	}

	// コンストラクタ
	FFGrid::FFGrid(const std::vector<double> &width){
		m_Width = width;
	}

	// グリッドの付随情報を計算する
	void FFGrid::precompute(bool periodic){
		// グリッド上でのセル間隔を計算する
		m_MidWidth.resize(m_Width.size() + 1);
		for (index_t i = 1; i < (index_t)m_Width.size(); i++){
			m_MidWidth[i] = (m_Width[i - 1] + m_Width[i]) * 0.5;
		}
		if (periodic == false){
			m_MidWidth[0] = m_Width[0];
			m_MidWidth[m_MidWidth.size() - 1] = m_Width[m_Width.size() - 1];
		}
		else{
			m_MidWidth[0] = (m_Width[0] + m_Width[m_Width.size() - 1]) * 0.5;
			m_MidWidth[m_MidWidth.size() - 1] = m_MidWidth[0];
		}

		// グリッドの座標を計算する
		double p = 0.0;
		m_Pos.resize(m_Width.size() + 1);
		for (index_t i = 0; i < (index_t)m_Width.size(); i++){
			m_Pos[i] = p;
			p += m_Width[i];
		}
		m_Pos.back() = p;
	}

	// グリッド全体の最小間隔を取得する
	double FFGrid::minimumWidth(void) const{
		double min = m_Width[0];
		for (auto it : m_Width){
			min = std::min(min, it);
		}
		return min;
	}

	// 物理座標から論理座標を取得する
	double FFGrid::physicalToLogical(double ppos, bool *out_of_range){
		if (ppos <= m_Pos.front()){
			// グリッドの負の範囲外
			double lpos = (ppos - m_Pos.front()) / m_MidWidth.front();
			if (out_of_range != nullptr) *out_of_range = (lpos <= -0.5);
			return lpos;
		}
		else if (m_Pos.back() <= ppos){
			// グリッドの正の範囲外
			double lpos = (ppos - m_Pos.back()) / m_MidWidth.back();
			if (out_of_range != nullptr) *out_of_range = (0.5 <= lpos);
			return m_Width.size() + lpos;
		}
		else{
			// グリッドの範囲内
			for (size_t i = 0; i < m_Width.size(); i++){
				if (ppos <= m_Pos[i + 1]){
					double lpos = (ppos - m_Pos[i]) / m_Width[i];
					if (out_of_range != nullptr) *out_of_range = false;
					return i + lpos;
				}
			}
		}
		// 異常
		if (out_of_range != nullptr) *out_of_range = true;
		return std::numeric_limits<double>::quiet_NaN();
	}

	// 論理座標から物理座標を取得する
	double FFGrid::logicalToPhysical(double lpos, bool *out_of_range){
		if (lpos <= 0.0){
			// グリッドの負の範囲外
			double ppos = m_Pos.front() + lpos * m_MidWidth.front();
			if (out_of_range != nullptr) *out_of_range = (lpos <= -0.5);
			return lpos;
		}
		else if ((double)(m_Pos.size() - 1) <= lpos){
			// グリッドの正の範囲外
			lpos -= (m_Pos.size() - 1);
			if (out_of_range != nullptr) *out_of_range = (0.5 <= lpos);
			return m_Pos.back() + m_MidWidth.back() * lpos;
		}
		else{
			// グリッドの範囲内
			int i = (int)lpos;
			if (out_of_range != nullptr) *out_of_range = false;
			return m_Pos[i] + m_Width[i] * (lpos - i);
		}
		// 異常
		if (out_of_range != nullptr) *out_of_range = true;
		return std::numeric_limits<double>::quiet_NaN();
	}
}
