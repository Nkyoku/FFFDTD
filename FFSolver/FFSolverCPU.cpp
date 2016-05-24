#include "FFSolverCPU.h"
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef _WIN32
#include <intrin.h>
#elif __GNUC__
#include <cpuid.h>
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

	// ソルバーの名前を取得する
	std::string FFSolverCPU::getSolverName(void) const{
		std::string result("Generic CPU");
		struct REG_t{
			uint32_t eax, ebx, ecx, edx;
		};
		REG_t reg[3];
#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_X64))
		__cpuid((int*)&reg[0], 0x80000000);
		if (0x80000002 <= reg[0].eax){
			__cpuid((int*)&reg[0], 0x80000002);
			__cpuid((int*)&reg[1], 0x80000003);
			__cpuid((int*)&reg[2], 0x80000004);
			char* p = (char*)&reg;
			result = (char*)reg;
		}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__amd64__))
		__get_cpuid(0x80000000, &reg[0].eax, &reg[0].ebx, &reg[0].ecx, &reg[0].edx);
		if (0x80000002 <= reg[0].eax){
			__get_cpuid(0x80000002, &reg[0].eax, &reg[0].ebx, &reg[0].ecx, &reg[0].edx);
			__get_cpuid(0x80000003, &reg[1].eax, &reg[1].ebx, &reg[1].ecx, &reg[1].edx);
			__get_cpuid(0x80000004, &reg[2].eax, &reg[2].ebx, &reg[2].ecx, &reg[2].edx);
			char* p = (char*)&reg;
			result = (char*)reg;
		}
#endif
		return result;
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
	void FFSolverCPU::initializeMemory(const index3_t &size, const index3_t &offset_m, const index3_t &offset_n, const index3_t &range_m, const index3_t &range_n){
		FFSolver::initializeMemory(size, offset_m, offset_n, range_m, range_n);

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
		// 時間ドメインプローブの測定を行う
		for (int i = 0; i < (int)m_TDProbeList.size(); i++){
			Probe_t &probe = m_TDProbeList[i];
			index_t index = probe.index;
			real value;
			switch (probe.type){
			case EMType::Ex:
				value = m_Ex[index];
				break;
			case EMType::Ey:
				value = m_Ey[index];
				break;
			case EMType::Ez:
				value = m_Ez[index];
				break;
			case EMType::Hx:
				value = m_Hx[index];
				break;
			case EMType::Hy:
				value = m_Hy[index];
				break;
			case EMType::Hz:
				value = m_Hz[index];
				break;
			}
			m_TDProbeMeasurment[i][n] = value;
		}

		// ポートの出力値を計算する
		for (int i = 0; i < (int)m_PortList.size(); i++){
			FFPort *port = m_PortList[i];
			if (port != nullptr){
				port->calcValue(this, n);
			}
		}
	}

	// 電界を計算する
	void FFSolverCPU::calcEField(void){
		const rvec2 *Coef2List = m_Coef2List.data();
		const rvec3 *Coef3List = m_Coef3List.data();
		const cindex_t *ExCIndex = m_ExCIndex.data();
		const cindex_t *EyCIndex = m_EyCIndex.data();
		const cindex_t *EzCIndex = m_EzCIndex.data();
		real *Ex = m_Ex.data();
		real *Ey = m_Ey.data();
		real *Ez = m_Ez.data();
		const real *Hx = m_Hx.data();
		const real *Hy = m_Hy.data();
		const real *Hz = m_Hz.data();
		const int X = 1;
		const int Y = m_Size.x + 1;
		const int Z = (m_Size.x + 1) * (m_Size.y + 1);
		const int RangeMx = m_RangeM.x;
		const int RangeMy = m_RangeM.y;
		const int RangeMz = m_RangeM.z;
		const int RangeNx = m_RangeN.x;
		const int RangeNy = m_RangeN.y;
		const int RangeNz = m_RangeN.z;
		const int ExOffset = X * m_StartM.x + Y * m_StartN.y + Z * m_StartN.z;
		const int EyOffset = X * m_StartN.x + Y * m_StartM.y + Z * m_StartN.z;
		const int EzOffset = X * m_StartN.x + Y * m_StartN.y + Z * m_StartM.z;

		// Dx,Exを計算する
#pragma omp parallel for
		for (int riz = 0; riz < RangeNz; riz++){
			for (int riy = 0; riy < RangeNy; riy++){
				int index = ExOffset + Y * riy + Z * riz;
				for (int rix = 0; rix < RangeMx; rix++){
					const rvec3 &coef = Coef3List[ExCIndex[index]];
					Ex[index]
						= coef.x * Ex[index]
						+ coef.y * (Hz[index] - Hz[index - Y])
						- coef.z * (Hy[index] - Hy[index - Z]);
					index++;
				}
			}
		}

		// Dy,Eyを計算する
#pragma omp parallel for
		for (int riz = 0; riz < RangeNz; riz++){
			for (int riy = 0; riy < RangeMy; riy++){
				int index = EyOffset + Y * riy + Z * riz;
				for (int rix = 0; rix < RangeNx; rix++){
					const rvec3 &coef = Coef3List[EyCIndex[index]];
					Ey[index]
						= coef.x * Ey[index]
						+ coef.y * (Hx[index] - Hx[index - Z])
						- coef.z * (Hz[index] - Hz[index - X]);
					index++;
				}
			}
		}

		// Dz,Ezを計算する
#pragma omp parallel for
		for (int riz = 0; riz < RangeMz; riz++){
			for (int riy = 0; riy < RangeNy; riy++){
				int index = EzOffset + Y * riy + Z * riz;
				for (int rix = 0; rix < RangeNx; rix++){
					const rvec3 &coef = Coef3List[EzCIndex[index]];
					Ez[index]
						= coef.x * Ez[index]
						+ coef.y * (Hy[index] - Hy[index - X])
						- coef.z * (Hx[index] - Hx[index - Y]);
					index++;
				}
			}
		}

		rvec2 *PmlDx = m_PMLDx.data();
		rvec2 *PmlDy = m_PMLDy.data();
		rvec2 *PmlDz = m_PMLDz.data();
		const cindex2_t *PmlDxCIndex = m_PMLDxCIndex.data();
		const cindex2_t *PmlDyCIndex = m_PMLDyCIndex.data();
		const cindex2_t *PmlDzCIndex = m_PMLDzCIndex.data();
		const index_t *PmlDxIndex = m_PMLDxIndex.data();
		const index_t *PmlDyIndex = m_PMLDyIndex.data();
		const index_t *PmlDzIndex = m_PMLDzIndex.data();

		// PML Dx,Exを計算する
		const int NumOfPMLDx = m_NumOfPMLD.x;
#pragma omp parallel for
		for (int i = 0; i < NumOfPMLDx; i++){
			const cindex2_t &pml_cindex = PmlDxCIndex[i];
			int index = PmlDxIndex[i];
			const rvec2 &coef_dxy = Coef2List[pml_cindex.x];
			const rvec2 &coef_dxz = Coef2List[pml_cindex.y];
			const rvec2 &coef_ex = Coef2List[ExCIndex[index]];
			real dx_prev = PmlDx[i].x + PmlDx[i].y;
			PmlDx[i].x
				= coef_dxy.x * PmlDx[i].x
				+ coef_dxy.y * (Hz[index] - Hz[index - Y]);
			PmlDx[i].y
				= coef_dxz.x * PmlDx[i].y
				- coef_dxz.y * (Hy[index] - Hy[index - Z]);
			real dx_next = PmlDx[i].x + PmlDx[i].y;
			Ex[index] = coef_ex.x * Ex[index] + coef_ex.y * (dx_next - dx_prev);
		}

		// PML Dy,Eyを計算する
		const int NumOfPMLDy = m_NumOfPMLD.y;
#pragma omp parallel for
		for (int i = 0; i < NumOfPMLDy; i++){
			const cindex2_t &pml_cindex = m_PMLDyCIndex[i];
			int index = m_PMLDyIndex[i];
			const rvec2 &coef_dyz = Coef2List[pml_cindex.x];
			const rvec2 &coef_dyx = Coef2List[pml_cindex.y];
			const rvec2 &coef_ey = Coef2List[EyCIndex[index]];
			real dy_prev = PmlDy[i].x + PmlDy[i].y;
			PmlDy[i].x
				= coef_dyz.x * PmlDy[i].x
				+ coef_dyz.y * (Hx[index] - Hx[index - Z]);
			PmlDy[i].y
				= coef_dyx.x * PmlDy[i].y
				- coef_dyx.y * (Hz[index] - Hz[index - X]);
			real dy_next = PmlDy[i].x + PmlDy[i].y;
			Ey[index] = coef_ey.x * Ey[index] + coef_ey.y * (dy_next - dy_prev);
		}

		// PML Dz,Ezを計算する
		const int NumOfPMLDz = m_NumOfPMLD.z;
#pragma omp parallel for
		for (int i = 0; i < NumOfPMLDz; i++){
			const cindex2_t &pml_cindex = PmlDzCIndex[i];
			int index = m_PMLDzIndex[i];
			const rvec2 &coef_dzx = Coef2List[pml_cindex.x];
			const rvec2 &coef_dzy = Coef2List[pml_cindex.y];
			const rvec2 &coef_ez = Coef2List[EzCIndex[index]];
			real dz_prev = PmlDz[i].x + PmlDz[i].y;
			PmlDz[i].x
				= coef_dzx.x * PmlDz[i].x
				+ coef_dzx.y * (Hy[index] - Hy[index - X]);
			PmlDz[i].y
				= coef_dzy.x * PmlDz[i].y
				- coef_dzy.y * (Hx[index] - Hx[index - Y]);
			real dz_next = PmlDz[i].x + PmlDz[i].y;
			Ez[index] = coef_ez.x * Ez[index] + coef_ez.y * (dz_next - dz_prev);
		}
	}

	// 磁界を計算する
	void FFSolverCPU::calcHField(void){
		const rvec2 *Coef2List = m_Coef2List.data();
		const rvec3 *Coef3List = m_Coef3List.data();
		const cindex_t *HxCIndex = m_HxCIndex.data();
		const cindex_t *HyCIndex = m_HyCIndex.data();
		const cindex_t *HzCIndex = m_HzCIndex.data();
		const real *Ex = m_Ex.data();
		const real *Ey = m_Ey.data();
		const real *Ez = m_Ez.data();
		real *Hx = m_Hx.data();
		real *Hy = m_Hy.data();
		real *Hz = m_Hz.data();
		const int X = 1;
		const int Y = m_Size.x + 1;
		const int Z = (m_Size.x + 1) * (m_Size.y + 1);
		const int RangeMx = m_RangeM.x;
		const int RangeMy = m_RangeM.y;
		const int RangeMz = m_RangeM.z;
		const int RangeNx = m_RangeN.x;
		const int RangeNy = m_RangeN.y;
		const int RangeNz = m_RangeN.z;
		const int HxOffset = X * m_StartN.x + Y * m_StartM.y + Z * m_StartM.z;
		const int HyOffset = X * m_StartM.x + Y * m_StartN.y + Z * m_StartM.z;
		const int HzOffset = X * m_StartM.x + Y * m_StartM.y + Z * m_StartN.z;

		// Hxを計算する
#pragma omp parallel for
		for (int riz = 0; riz < RangeMz; riz++){
			for (int riy = 0; riy < RangeMy; riy++){
				int index = HxOffset + Y * riy + Z * riz;
				for (int rix = 0; rix < RangeNx; rix++){
					const rvec3 &coef = Coef3List[HxCIndex[index]];
					Hx[index]
						= coef.x * Hx[index]
						- coef.y * (Ez[index + Y] - Ez[index])
						+ coef.z * (Ey[index + Z] - Ey[index]);
					index++;
				}
			}
		}

		// Hyを計算する
#pragma omp parallel for
		for (int riz = 0; riz < RangeMz; riz++){
			for (int riy = 0; riy < RangeNy; riy++){
				int index = HyOffset + Y * riy + Z * riz;
				for (int rix = 0; rix < RangeMx; rix++){
					const rvec3 &coef = Coef3List[HyCIndex[index]];
					Hy[index]
						= coef.x * Hy[index]
						- coef.y * (Ex[index + Z] - Ex[index])
						+ coef.z * (Ez[index + X] - Ez[index]);
					index++;
				}
			}
		}

		// Hzを計算する
#pragma omp parallel for
		for (int riz = 0; riz < RangeNz; riz++){
			for (int riy = 0; riy < RangeMy; riy++){
				int index = HzOffset + Y * riy + Z * riz;
				for (int rix = 0; rix < RangeMx; rix++){
					const rvec3 &coef = Coef3List[HzCIndex[index]];
					Hz[index]
						= coef.x * Hz[index]
						- coef.y * (Ey[index + X] - Ey[index])
						+ coef.z * (Ex[index + Y] - Ex[index]);
					index++;
				}
			}
		}

		rvec2 *PmlHx = m_PMLHx.data();
		rvec2 *PmlHy = m_PMLHy.data();
		rvec2 *PmlHz = m_PMLHz.data();
		const cindex2_t *PmlHxCIndex = m_PMLHxCIndex.data();
		const cindex2_t *PmlHyCIndex = m_PMLHyCIndex.data();
		const cindex2_t *PmlHzCIndex = m_PMLHzCIndex.data();
		const index_t *PmlHxIndex = m_PMLHxIndex.data();
		const index_t *PmlHyIndex = m_PMLHyIndex.data();
		const index_t *PmlHzIndex = m_PMLHzIndex.data();

		// PML Hxを計算する
		const int NumOfPMLHx = m_NumOfPMLH.x;
#pragma omp parallel for
		for (int i = 0; i < NumOfPMLHx; i++){
			const cindex2_t &pml_cindex = PmlHxCIndex[i];
			int index = PmlHxIndex[i];
			const rvec2 &coef_hxy = Coef2List[pml_cindex.x];
			const rvec2 &coef_hxz = Coef2List[pml_cindex.y];
			PmlHx[i].x
				= coef_hxy.x * PmlHx[i].x
				- coef_hxy.y * (Ez[index + Y] - Ez[index]);
			PmlHx[i].y
				= coef_hxz.x * PmlHx[i].y
				+ coef_hxz.y * (Ey[index + Z] - Ey[index]);
			Hx[index] = PmlHx[i].x + PmlHx[i].y;
		}

		// PML Hyを計算する
		const int NumOfPMLHy = m_NumOfPMLH.y;
#pragma omp parallel for
		for (int i = 0; i < NumOfPMLHy; i++){
			const cindex2_t &pml_cindex = PmlHyCIndex[i];
			int index = PmlHyIndex[i];
			const rvec2 &coef_hyz = Coef2List[pml_cindex.x];
			const rvec2 &coef_hyx = Coef2List[pml_cindex.y];
			PmlHy[i].x
				= coef_hyz.x * PmlHy[i].x
				- coef_hyz.y * (Ex[index + Z] - Ex[index]);
			PmlHy[i].y
				= coef_hyx.x * PmlHy[i].y
				+ coef_hyx.y * (Ez[index + X] - Ez[index]);
			Hy[index] = PmlHy[i].x + PmlHy[i].y;
		}

		// PML Hzを計算する
		const int NumOfPMLHz = m_NumOfPMLH.z;
#pragma omp parallel for
		for (int i = 0; i < NumOfPMLHz; i++){
			const cindex2_t &pml_cindex = PmlHzCIndex[i];
			int index = PmlHzIndex[i];
			const rvec2 &coef_hzx = Coef2List[pml_cindex.x];
			const rvec2 &coef_hzy = Coef2List[pml_cindex.y];
			PmlHz[i].x
				= coef_hzx.x * PmlHz[i].x
				- coef_hzx.y * (Ey[index + X] - Ey[index]);
			PmlHz[i].y
				= coef_hzy.x * PmlHz[i].y
				+ coef_hzy.y * (Ex[index + Y] - Ex[index]);
			Hz[index] = PmlHz[i].x + PmlHz[i].y;
		}
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
	void FFSolverCPU::getEdgeE(const real **top_ex, const real **top_ey, const real **bottom_ez) const{
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (top_ex != nullptr){
			*top_ex = m_Ex.data() + Z * m_Size.z;
		}
		if (top_ey != nullptr){
			*top_ey = m_Ey.data() + Z * m_Size.z;
		}
		if (bottom_ez != nullptr){
			*bottom_ez = m_Ez.data();
		}
	}

	// Z端部の電界を設定する
	void FFSolverCPU::setEdgeE(const real *bottom_ex, const real *bottom_ey, const real *top_ez){
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (bottom_ex != nullptr){
			memcpy(m_Ex.data(), bottom_ex, sizeof(real) * Z);
		}
		if (bottom_ey != nullptr){
			memcpy(m_Ey.data(), bottom_ey, sizeof(real) * Z);
		}
		if (top_ez != nullptr){
			memcpy(m_Ez.data() + Z * m_Size.z, top_ez, sizeof(real) * Z);
		}
	}

	// Z端部の磁界を取得する
	void FFSolverCPU::getEdgeH(const real **bottom_hx, const real **bottom_hy, const real **top_hz) const{
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (bottom_hx != nullptr){
			*bottom_hx = m_Hx.data();
		}
		if (bottom_hy != nullptr){
			*bottom_hy = m_Hy.data();
		}
		if (top_hz != nullptr){
			*top_hz = m_Hz.data() + Z * m_Size.z;
		}
	}

	// Z端部の磁界を設定する
	void FFSolverCPU::setEdgeH(const real *top_hx, const real *top_hy, const real *bottom_hz){
		const index_t Z = (m_Size.x + 1) * (m_Size.y + 1);
		if (top_hx != nullptr){
			memcpy(m_Hx.data() + Z * m_Size.z, top_hx, sizeof(real) * Z);
		}
		if (top_hy != nullptr){
			memcpy(m_Hy.data() + Z * m_Size.z, top_hy, sizeof(real) * Z);
		}
		if (bottom_hz != nullptr){
			memcpy(m_Hz.data(), bottom_hz, sizeof(real) * Z);
		}
	}




}
