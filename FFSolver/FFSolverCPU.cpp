#include "FFSolverCPU.h"
#ifdef _OPENMP
#include <omp.h>
#endif



namespace FFFDTD{
	// ソルバーを作成する
	FFSolverCPU* FFSolverCPU::createSolver(int number_of_threads){
		return new FFSolverCPU(number_of_threads);
	}

	// コンストラクタ
	FFSolverCPU::FFSolverCPU(int number_of_threads) : FFSolver(){

#ifdef _OPENMP
		// 並列スレッド数を指定する
		if (0 < number_of_threads){
			omp_set_num_threads(number_of_threads);
		}
#endif


	}

	// デストラクタ
	FFSolverCPU::~FFSolverCPU(){

	}

	// 電界・磁界の絶対合計値を計算する
	dvec2 FFSolverCPU::calcTotalEM(void){
		return dvec2(0.0, 0.0);
	}

	// 電磁界成分を格納するメモリーを確保し初期化する
	void FFSolverCPU::initializeMemory(index_t x, index_t y, index_t z){

	}

	// 係数インデックスを格納する
	void FFSolverCPU::storeCoefficientIndex(COEFFICIENT_e type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index){

	}

	// 係数リストを格納する
	void FFSolverCPU::storeCoefficientList(const std::vector<rvec2> &coef2_list, const std::vector<rvec3> &coef3_list){

	}

	// 給電と観測を行う
	void FFSolverCPU::feedAndMeasure(void){

	}

	// 電界を計算する
	void FFSolverCPU::calcEField(void){

	}

	// 磁界を計算する
	void FFSolverCPU::calcHField(void){

	}


}
