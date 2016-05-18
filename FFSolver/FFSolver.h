#pragma once

#include "FFType.h"
#include "FFGrid.h"
#include "FFMaterial.h"
#include "FFPort.h"



namespace FFFDTD{
	// シミュレーションを行う基底クラス
	class FFSolver{
		friend class FFPort;

		/*** 定数 ***/
	public:
		


		/*** 定義 ***/
	protected:
		


		/*** メンバー変数 ***/
	protected:
		// 空間のサイズ
		index3_t m_Size;

		// 通常空間のオフセット
		index3_t m_NormalOffset;

		// 通常空間のサイズ
		index3_t m_NormalSize;

		// 通常空間の計算範囲
		index3_t m_StartM, m_StartN, m_RangeM, m_RangeN;

		// PML空間の成分数
		index3_t m_NumOfPMLD, m_NumOfPMLH;

		// 解析角周波数のリスト
		std::vector<double> m_OmegaList;
		
		// ポートのリスト
		std::vector<FFPort*> m_PortList;

		// 時間ドメインプローブのリスト
		std::vector<Probe_t> m_TDProbeList;

		// 周波数ドメインプローブのリスト
		std::vector<Probe_t> m_FDProbeList;

		// 時間ドメインプローブの測定値
		std::vector<std::vector<real>> m_TDProbeMeasurment;

		// 周波数ドメインプローブの測定値
		std::vector<std::vector<double>> m_FDProbeMeasurment;



		/*** メソッド ***/
	protected:
		// コンストラクタ
		FFSolver(void);

	public:
		// デストラクタ
		virtual ~FFSolver(){}

		// 空間のサイズを取得する
		const index3_t& getSize(void) const{
			return m_Size;
		}

		// 電界・磁界の絶対合計値を計算する
		virtual dvec2 calcTotalEM(void) = 0;

		// 電磁界成分を格納するメモリーを確保し初期化する
		virtual void initializeMemory(const index3_t &size, const index3_t &offset_m, const index3_t &offset_n, const index3_t &range_m, const index3_t &range_n);

		// 係数インデックスを格納する
		virtual void storeCoefficientIndex(EMType type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index);

		// 係数リストを格納する
		virtual void storeCoefficientList(const std::vector<rvec2> &coef2_list, const std::vector<rvec3> &coef3_list) = 0;

		// 観測に関する情報を格納する
		virtual void storeMeasurementInfo(const std::vector<double> &freq_list, size_t max_iteration, const std::vector<Probe_t> &td_probe_list, const std::vector<Probe_t> &fd_probe_list);

		// ポートリストを格納する
		virtual void storePortList(const std::vector<FFPort*> &port_list);





		// 給電と観測を行う
		virtual void feedAndMeasure(size_t n) = 0;

		// 電界を計算する
		virtual void calcEField(void) = 0;

		// 磁界を計算する
		virtual void calcHField(void) = 0;

		// 端部の電界を交換する
		virtual void exchangeEdgeE(Axis axis) = 0;

		// 端部の磁界を交換する
		virtual void exchangeEdgeH(Axis axis) = 0;

		// Z端部の電界を取得する
		virtual void getEdgeE(const real **top_ex, const real **top_ey, const real **bottom_ez) const = 0;
		
		// Z端部の電界を設定する
		virtual void setEdgeE(const real *bottom_ex, const real *bottom_ey, const real *top_ez) = 0;

		// Z端部の磁界を取得する
		virtual void getEdgeH(const real **bottom_hx, const real **bottom_hy, const real **top_hz) const = 0;

		// Z端部の磁界を設定する
		virtual void setEdgeH(const real *top_hx, const real *top_hy, const real *bottom_hz) = 0;

	protected:
		// プローブの観測値を取得する
		double getProbeValue(oindex_t id, size_t n, ProbeType type) const{
			if (type == ProbeType::TD){
				return m_TDProbeMeasurment[id][n];
			}
			else if (type == ProbeType::FD){
				return m_FDProbeMeasurment[id][n];
			}
			else{
				throw;
			}
		}

		// 時間ドメインプローブの位置の電磁界を励振する
		virtual void setTDProbeValue(oindex_t id, real value) = 0;



	};
}
