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
		const index_t Nx = m_Size.x + 1;
		const index_t Ny = m_Size.y + 1;
		const index_t Nz = m_Size.z + 1;
		const index_t Y = Nx;
		const index_t Z = Nx * Ny;
		real *Ex = m_Ex.data();
		real *Ey = m_Ey.data();
		real *Ez = m_Ez.data();
		real *Hx = m_Hx.data();
		real *Hy = m_Hy.data();
		real *Hz = m_Hz.data();

		double e_total = 0.0, h_total = 0.0;
#pragma omp parallel for reduction(+ : e_total, h_total)
		for (int iz_ = 0; (index_t)iz_ < Nz; iz_++){
			index_t iz = (index_t)iz_;
			for (index_t iy = 0; iy < Ny; iy++){
				for (index_t ix = 0; ix < Nx; ix++){
					e_total += abs(Ex[ix + Y * iy + Z * iz]);
					e_total += abs(Ey[ix + Y * iy + Z * iz]);
					e_total += abs(Ez[ix + Y * iy + Z * iz]);
					h_total += abs(Hx[ix + Y * iy + Z * iz]);
					h_total += abs(Hy[ix + Y * iy + Z * iz]);
					h_total += abs(Hz[ix + Y * iy + Z * iz]);
				}
			}
		}

		return dvec2(e_total, h_total);
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
	void FFSolverCPU::feedAndMeasure(size_t n){

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

	// 端部の電界を交換する
	void FFSolverCPU::exchangeEdgeE(Axis axis){
		const index_t Mx = m_Size.x;
		const index_t My = m_Size.y;
		const index_t Mz = m_Size.z;
		const index_t Nx = m_Size.x + 1;
		const index_t Ny = m_Size.y + 1;
		const index_t Nz = m_Size.z + 1;
		const index_t Y = Nx;
		const index_t Z = Nx * Ny;
		real *Ex = m_Ex.data();
		real *Ey = m_Ey.data();
		real *Ez = m_Ez.data();
		if (axis == Axis::X){
			for (index_t iz = 0; iz < Nz; iz++){
				for (index_t iy = 0; iy < Ny; iy++){
					Ex[Mx + Y * iy + Z * iz] = Ex[0 + Y * iy + Z * iz];
					Ey[0 + Y * iy + Z * iz] = Ey[Mx + Y * iy + Z * iz];
					Ez[0 + Y * iy + Z * iz] = Ez[Mx + Y * iy + Z * iz];
				}
			}
		}
		else if (axis == Axis::Y){
			for (index_t iz = 0; iz < Nz; iz++){
				for (index_t ix = 0; ix < Nx; ix++){
					Ex[ix + Y * 0 + Z * iz] = Ex[ix + Y * My + Z * iz];
					Ey[ix + Y * My + Z * iz] = Ey[ix + Y * 0 + Z * iz];
					Ez[ix + Y * 0 + Z * iz] = Ez[ix + Y * My + Z * iz];
				}
			}
		}
		else if (axis == Axis::Z){
			memcpy(Ex, Ex + Z * Mz, sizeof(real) * Nx * Ny);
			memcpy(Ey, Ey + Z * Mz, sizeof(real) * Nx * Ny);
			memcpy(Ez + Z * Mz, Ez, sizeof(real) * Nx * Ny);
		}
	}

	// 端部の磁界を交換する
	void FFSolverCPU::exchangeEdgeH(Axis axis){
		const index_t Mx = m_Size.x;
		const index_t My = m_Size.y;
		const index_t Mz = m_Size.z;
		const index_t Nx = m_Size.x + 1;
		const index_t Ny = m_Size.y + 1;
		const index_t Nz = m_Size.z + 1;
		const index_t Y = Nx;
		const index_t Z = Nx * Ny;
		real *Hx = m_Hx.data();
		real *Hy = m_Hy.data();
		real *Hz = m_Hz.data();
		if (axis == Axis::X){
			for (index_t iz = 0; iz < Nz; iz++){
				for (index_t iy = 0; iy < Ny; iy++){
					Hx[0 + Y * iy + Z * iz] = Hx[Mx + Y * iy + Z * iz];
					Hy[Mx + Y * iy + Z * iz] = Hy[0 + Y * iy + Z * iz];
					Hz[Mx + Y * iy + Z * iz] = Hz[0 + Y * iy + Z * iz];
				}
			}
		}
		else if (axis == Axis::Y){
			for (index_t iz = 0; iz < Nz; iz++){
				for (index_t ix = 0; ix < Nx; ix++){
					Hx[ix + Y * My + Z * iz] = Hx[ix + Y * 0 + Z * iz];
					Hy[ix + Y * 0 + Z * iz] = Hy[ix + Y * My + Z * iz];
					Hz[ix + Y * My + Z * iz] = Hz[ix + Y * 0 + Z * iz];
				}
			}
		}
		else if (axis == Axis::Z){
			memcpy(Hx + Z * Mz, Hx, sizeof(real) * Z);
			memcpy(Hy + Z * Mz, Hy, sizeof(real) * Z);
			memcpy(Hz, Hz + Z * Mz, sizeof(real) * Z);
		}
	}

	// Z端部の電界を取得する
	void FFSolverCPU::getEdgeE(std::vector<real> *top_ex, std::vector<real> *top_ey, std::vector<real> *bottom_ez) const{
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (top_ex != nullptr){
			top_ex->resize(Z);
			memcpy(top_ex->data(), m_Ex.data() + Z * m_Size.z, sizeof(real) * Z);
		}
		if (top_ey != nullptr){
			top_ey->resize(Z);
			memcpy(top_ey->data(), m_Ey.data() + Z * m_Size.z, sizeof(real) * Z);
		}
		if (bottom_ez != nullptr){
			bottom_ez->resize(Z);
			memcpy(bottom_ez->data(), m_Ez.data(), sizeof(real) * Z);
		}
	}

	// Z端部の電界を設定する
	void FFSolverCPU::setEdgeE(const std::vector<real> *bottom_ex, const std::vector<real> *bottom_ey, const std::vector<real> *top_ez){
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (bottom_ex != nullptr){
			memcpy(m_Ex.data(), bottom_ex->data(), sizeof(real) * Z);
		}
		if (bottom_ey != nullptr){
			memcpy(m_Ey.data(), bottom_ey->data(), sizeof(real) * Z);
		}
		if (top_ez != nullptr){
			memcpy(m_Ez.data() + Z * m_Size.z, top_ez->data(), sizeof(real) * Z);
		}
	}

	// Z端部の磁界を取得する
	void FFSolverCPU::getEdgeH(std::vector<real> *bottom_hx, std::vector<real> *bottom_hy, std::vector<real> *top_hz) const{
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (bottom_hx != nullptr){
			bottom_hx->resize(Z);
			memcpy(bottom_hx->data(), m_Ex.data(), sizeof(real) * Z);
		}
		if (bottom_hy != nullptr){
			bottom_hy->resize(Z);
			memcpy(bottom_hy->data(), m_Ey.data(), sizeof(real) * Z);
		}
		if (top_hz != nullptr){
			top_hz->resize(Z);
			memcpy(top_hz->data(), m_Hz.data() + Z * m_Size.z, sizeof(real) * Z);
		}
	}

	// Z端部の磁界を設定する
	void FFSolverCPU::setEdgeH(const std::vector<real> *top_hx, const std::vector<real> *top_hy, const std::vector<real> *bottom_hz){
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (top_hx != nullptr){
			memcpy(m_Hx.data() + Z * m_Size.z, top_hx->data(), sizeof(real) * Z);
		}
		if (top_hy != nullptr){
			memcpy(m_Hy.data() + Z * m_Size.z, top_hy->data(), sizeof(real) * Z);
		}
		if (bottom_hz != nullptr){
			memcpy(m_Hz.data(), bottom_hz->data(), sizeof(real) * Z);
		}
	}




}
