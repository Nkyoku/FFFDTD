#pragma once

#include "FFSolver.h"



namespace FFFDTD{
	// CPUで計算するソルバーのクラス
	class FFSolverCPU : public FFSolver{
		/*** メンバー変数 ***/
	private:
		


		/*** メソッド ***/
	public:
		// ソルバーを作成する
		static FFSolverCPU* createSolver(int number_of_threads);

	private:
		// コンストラクタ
		FFSolverCPU(int number_of_threads);

	public:
		// デストラクタ
		~FFSolverCPU();

		// 電界・磁界の絶対合計値を計算する
		dvec2 calcTotalEM(void) override;

		// 電磁界成分を格納するメモリーを確保し初期化する
		void initializeMemory(index_t x, index_t y, index_t z) override;

		// 係数インデックスを格納する
		void storeCoefficientIndex(COEFFICIENT_e type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index) override;

		// 係数リストを格納する
		void storeCoefficientList(const std::vector<rvec2> &coef2_list, const std::vector<rvec3> &coef3_list) override;

		// 給電と観測を行う
		void feedAndMeasure(void) override;

		// 電界を計算する
		void calcEField(void) override;

		// 磁界を計算する
		void calcHField(void) override;








	};
}
