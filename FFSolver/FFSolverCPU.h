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

		// 電界・磁界の絶対合計値を計算する
		dvec2 calcTotalEM(void) override;

		// 電磁界成分を格納するメモリーを確保し初期化する
		void initializeMemory(const index3_t &size, const index3_t &normal_offset, const index3_t &normal_size) override;

		// 係数インデックスを格納する
		void storeCoefficientIndex(EMType type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index) override;

		// 係数リストを格納する
		void storeCoefficientList(const std::vector<rvec2> &coef2_list, const std::vector<rvec3> &coef3_list) override;

		// 給電と観測を行う
		void feedAndMeasure(void) override;

		// 電界を計算する
		void calcEField(void) override;

		// 磁界を計算する
		void calcHField(void) override;

	protected:
		// 時間ドメインプローブの位置の電磁界を励振する
		void setTDProbeValue(oindex_t id, real value) override;
	};
}
