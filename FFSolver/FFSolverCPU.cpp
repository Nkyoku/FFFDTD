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
	FFSolverCPU::FFSolverCPU(int number_of_threads)
		: FFSolver()
	{

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
	void FFSolverCPU::initializeMemory(const index3_t &size, const index3_t &normal_offset, const index3_t &normal_size){
		FFSolver::initializeMemory(size, normal_offset, normal_size);

		// メモリーを確保する
		size_t volume = (size_t)(size.x + 1) * (size_t)(size.y + 1) * (size_t)(size.z + 1);
		m_Ex.resize(volume, 0.0);
		m_Ey.resize(volume, 0.0);
		m_Ez.resize(volume, 0.0);
		m_Hx.resize(volume, 0.0);
		m_Hy.resize(volume, 0.0);
		m_Hz.resize(volume, 0.0);
	}

	// 係数インデックスを格納する
	void FFSolverCPU::storeCoefficientIndex(EMType type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index){
		FFSolver::storeCoefficientIndex(type, normal_cindex, pml_cindex, pml_index);
		
		switch (type){
		case EMType::Ex:
			m_ExCIndex = normal_cindex;
			m_PMLDxCIndex = pml_cindex;
			m_PMLDxIndex = pml_index;
			m_PMLDx.resize(pml_cindex.size(), rvec2(0.0, 0.0));
			break;

		case EMType::Ey:
			m_EyCIndex = normal_cindex;
			m_PMLDyCIndex = pml_cindex;
			m_PMLDyIndex = pml_index;
			m_PMLDy.resize(pml_cindex.size(), rvec2(0.0, 0.0));
			break;

		case EMType::Ez:
			m_EzCIndex = normal_cindex;
			m_PMLDzCIndex = pml_cindex;
			m_PMLDzIndex = pml_index;
			m_PMLDz.resize(pml_cindex.size(), rvec2(0.0, 0.0));
			break;

		case EMType::Hx:
			m_HxCIndex = normal_cindex;
			m_PMLHxCIndex = pml_cindex;
			m_PMLHxIndex = pml_index;
			m_PMLHx.resize(pml_cindex.size(), rvec2(0.0, 0.0));
			break;

		case EMType::Hy:
			m_HyCIndex = normal_cindex;
			m_PMLHyCIndex = pml_cindex;
			m_PMLHyIndex = pml_index;
			m_PMLHy.resize(pml_cindex.size(), rvec2(0.0, 0.0));
			break;

		case EMType::Hz:
			m_HzCIndex = normal_cindex;
			m_PMLHzCIndex = pml_cindex;
			m_PMLHzIndex = pml_index;
			m_PMLHz.resize(pml_cindex.size(), rvec2(0.0, 0.0));
			break;
		}
	}

	// 係数リストを格納する
	void FFSolverCPU::storeCoefficientList(const std::vector<rvec2> &coef2_list, const std::vector<rvec3> &coef3_list){
		m_Coef2List = coef2_list;
		m_Coef3List = coef3_list;
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
	
	// 時間ドメインプローブの位置の電磁界を励振する
	void FFSolverCPU::setTDProbeValue(oindex_t id, real value){
		// 電磁界の値を反映する
		Probe_t &probe = m_TDProbeList[id];
		switch (probe.type){
		case EMType::Ex:
			m_Ex[probe.index] = value;
			break;

		case EMType::Ey:
			m_Ey[probe.index] = value;
			break;

		case EMType::Ez:
			m_Ez[probe.index] = value;
			break;

		case EMType::Hx:
			m_Hx[probe.index] = value;
			break;

		case EMType::Hy:
			m_Hy[probe.index] = value;
			break;

		case EMType::Hz:
			m_Hz[probe.index] = value;
			break;
		}
	}

	



}
