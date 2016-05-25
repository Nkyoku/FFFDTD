#pragma once

#include "FFSolver.h"



namespace FFFDTD{
	// CPUで計算するソルバーのクラス
	class FFSolverCPU : public FFSolver{
		/*** メンバー変数 ***/
	private:
		// 電界
		std::vector<real> m_Ex, m_Ey, m_Ez;
		std::vector<cindex_t> m_ExCIndex, m_EyCIndex, m_EzCIndex;

		// 磁界
		std::vector<real> m_Hx, m_Hy, m_Hz;
		std::vector<cindex_t> m_HxCIndex, m_HyCIndex, m_HzCIndex;

		// PML電束密度
		std::vector<rvec2> m_PMLDx, m_PMLDy, m_PMLDz;
		std::vector<cindex2_t> m_PMLDxCIndex, m_PMLDyCIndex, m_PMLDzCIndex;
		std::vector<index_t> m_PMLDxIndex, m_PMLDyIndex, m_PMLDzIndex;

		// PML磁界
		std::vector<rvec2> m_PMLHx, m_PMLHy, m_PMLHz;
		std::vector<cindex2_t> m_PMLHxCIndex, m_PMLHyCIndex, m_PMLHzCIndex;
		std::vector<index_t> m_PMLHxIndex, m_PMLHyIndex, m_PMLHzIndex;

		// 2組係数のリスト
		std::vector<rvec2> m_Coef2List;

		// 3組係数のリスト
		std::vector<rvec3> m_Coef3List;



		/*** メソッド ***/
	public:
		// ソルバーを作成する
		static FFSolverCPU* createSolver(int number_of_threads = 0);

	private:
		// コンストラクタ
		FFSolverCPU(int number_of_threads);

	public:
		// デストラクタ
		~FFSolverCPU();

		// ソルバーの名前を取得する
		std::string getSolverName(void) const override;

		// 電界・磁界の絶対合計値を計算する
		dvec2 calcTotalEM(void) override;

		// 電磁界成分を格納するメモリーを確保し初期化する
		void initializeMemory(const index3_t &size, const index3_t &offset_m, const index3_t &offset_n, const index3_t &range_m, const index3_t &range_n) override;

		// 係数インデックスを格納する
		void storeCoefficientIndex(EMType type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index) override;

		// 係数リストを格納する
		void storeCoefficientList(const std::vector<rvec2> &coef2_list, const std::vector<rvec3> &coef3_list) override;

		// 給電と観測を行う
		void feedAndMeasure(size_t n) override;

		// 電界を計算する
		void calcEField(void) override;

		// 磁界を計算する
		void calcHField(void) override;

		// 端部の電界を交換する
		void exchangeEdgeE(Axis axis) override;

		// 端部の磁界を交換する
		void exchangeEdgeH(Axis axis) override;

		// Z端部の電界を取得する
		void getEdgeE(const real **top_ex, const real **top_ey, const real **bottom_ez) const override;

		// Z端部の電界を設定する
		void setEdgeE(const real *bottom_ex, const real *bottom_ey, const real *top_ez) override;

		// Z端部の磁界を取得する
		void getEdgeH(const real **bottom_hx, const real **bottom_hy, const real **top_hz) const override;

		// Z端部の磁界を設定する
		void setEdgeH(const real *top_hx, const real *top_hy, const real *bottom_hz) override;
		
	protected:
		// 時間ドメインプローブの位置の電磁界を励振する
		void setTDProbeValue(oindex_t id, real value) override;

	public:
		// デバッグ用に指定した座標のEx成分を取得する
		real getExDebug(index_t x, index_t y, index_t z) const{
			const index_t Y = m_Size.x + 1;
			const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
			return m_Ex[x + Y * y + Z * z];
		}

		// デバッグ用に指定した座標のEy成分を取得する
		real getEyDebug(index_t x, index_t y, index_t z) const{
			const index_t Y = m_Size.x + 1;
			const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
			return m_Ey[x + Y * y + Z * z];
		}

		// デバッグ用に指定した座標のEz成分を取得する
		real getEzDebug(index_t x, index_t y, index_t z) const{
			const index_t Y = m_Size.x + 1;
			const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
			return m_Ez[x + Y * y + Z * z];
		}

		// デバッグ用に指定した座標のHx成分を取得する
		real getHxDebug(index_t x, index_t y, index_t z) const{
			const index_t Y = m_Size.x + 1;
			const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
			return m_Hx[x + Y * y + Z * z];
		}

		// デバッグ用に指定した座標のHy成分を取得する
		real getHyDebug(index_t x, index_t y, index_t z) const{
			const index_t Y = m_Size.x + 1;
			const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
			return m_Hy[x + Y * y + Z * z];
		}

		// デバッグ用に指定した座標のHz成分を取得する
		real getHzDebug(index_t x, index_t y, index_t z) const{
			const index_t Y = m_Size.x + 1;
			const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
			return m_Hz[x + Y * y + Z * z];
		}
	};
}
