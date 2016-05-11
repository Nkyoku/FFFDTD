#include "FFSolver.h"
#include <algorithm>



namespace FFFDTD{

	// コンストラクタ
	FFSolver::FFSolver(void)
		: m_Size(0, 0, 0), m_NormalOffset(0, 0, 0), m_NormalSize(0, 0, 0)
		, m_StartM(0, 0, 0), m_StartN(0, 0, 0), m_RangeM(0, 0, 0), m_RangeN(0, 0, 0)
		, m_NumOfPMLD(0, 0, 0), m_NumOfPMLH(0, 0, 0)
		, m_OmegaList()
		, m_PortList()
		, m_TDProbeList(), m_FDProbeList()
		, m_TDProbeMeasurment(), m_FDProbeMeasurment()
	{

	}

	// 電磁界成分を格納するメモリーを確保し初期化する
	void FFSolver::initializeMemory(const index3_t &size, const index3_t &normal_offset, const index3_t &normal_size){
		m_StartM.x = normal_offset.x;
		m_StartN.x = normal_offset.x + 1;
		m_StartM.y = normal_offset.y;
		m_StartN.y = normal_offset.y + 1;
		m_StartM.z = normal_offset.z;
		m_StartN.z = normal_offset.z + 1;
		m_RangeM.x = normal_size.x;
		m_RangeN.x = normal_size.x;
		m_RangeM.y = normal_size.y;
		m_RangeN.y = normal_size.y;
		m_RangeM.z = normal_size.z;
		m_RangeN.z = normal_size.z;
	}

	// 係数インデックスを格納する
	void FFSolver::storeCoefficientIndex(EMType type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index){
		switch (type){
		case EMType::Ex:
			m_NumOfPMLD.x = (index_t)pml_cindex.size();
			break;
		case EMType::Ey:
			m_NumOfPMLD.y = (index_t)pml_cindex.size();
			break;
		case EMType::Ez:
			m_NumOfPMLD.z = (index_t)pml_cindex.size();
			break;
		case EMType::Hx:
			m_NumOfPMLH.x = (index_t)pml_cindex.size();
			break;
		case EMType::Hy:
			m_NumOfPMLH.y = (index_t)pml_cindex.size();
			break;
		case EMType::Hz:
			m_NumOfPMLH.z = (index_t)pml_cindex.size();
			break;
		}
	}

	// 観測に関する情報を格納する
	void FFSolver::storeMeasurementInfo(const std::vector<double> &freq_list, size_t max_iteration, const std::vector<Probe_t> &td_probe_list, const std::vector<Probe_t> &fd_probe_list){
		m_OmegaList.resize(freq_list.size());
		for (size_t i = 0; i < freq_list.size(); i++){
			m_OmegaList[i] = 2.0 * PI * freq_list[i];
		}

		m_TDProbeList = td_probe_list;
		m_FDProbeList = fd_probe_list;

		m_TDProbeMeasurment.resize(td_probe_list.size());
		for (auto &it : m_TDProbeMeasurment){
			it.resize(max_iteration, 0.0);
		}

		m_FDProbeMeasurment.resize(fd_probe_list.size());
		for (auto &it : m_FDProbeMeasurment){
			it.resize(freq_list.size(), 0.0);
		}
	}

	// ポートリストを格納する
	void FFSolver::storePortList(const std::vector<FFPort*> &port_list){
		m_PortList = port_list;
	}
}


