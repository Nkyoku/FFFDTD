#pragma once

#include "FFType.h"
#include "FFGrid.h"
#include "FFMaterial.h"
#include "FFPort.h"



namespace FFFDTD{
	// シミュレーションを行う基底クラス
	class FFSolver{
		/*** 定数 ***/
	public:
		// 係数の種類
		enum COEFFICIENT_e{
			COEF_EX,
			COEF_EY,
			COEF_EZ,
			COEF_HX,
			COEF_HY,
			COEF_HZ
		};



		/*** 定義 ***/
	protected:
		


		/*** メンバー変数 ***/
	protected:
		// 空間のサイズ
		index3_t m_Size;
		
		// 通常空間のオフセット
		index3_t m_Offset;
		
		// 2組係数のリスト
		std::vector<rvec2> m_Coef2List;

		// 3組係数のリスト
		std::vector<rvec3> m_Coef3List;

		// 解析角周波数のリスト
		std::vector<double> m_OmegaList;




		// ポートのリスト
		//std::vector<DPort*> m_PortList;

		// 周波数プローブのリスト
		//std::vector<DFreqProbeInfo*> m_FreqProbeList;

		// 時間プローブのリスト
		//std::vector<DProbe*> m_TimeProbeList;

		// 通常空間の計算範囲
		int m_StartMx, m_StartMy, m_StartMz;
		int m_StartNx, m_StartNy, m_StartNz;
		int m_RangeMx, m_RangeMy, m_RangeMz;
		int m_RangeNx, m_RangeNy, m_RangeNz;

		// PML空間の成分数
		int m_NumOfPMLDx, m_NumOfPMLDy, m_NumOfPMLDz;
		int m_NumOfPMLHx, m_NumOfPMLHy, m_NumOfPMLHz;
		


		/*** メソッド ***/
	protected:
		// コンストラクタ
		FFSolver(void);

	public:
		// デストラクタ
		virtual ~FFSolver(){}

		// 電界・磁界の絶対合計値を計算する
		virtual dvec2 calcTotalEM(void) = 0;

		// 電磁界成分を格納するメモリーを確保し初期化する
		virtual void initializeMemory(index_t x, index_t y, index_t z) = 0;

		// 係数インデックスを格納する
		virtual void storeCoefficientIndex(COEFFICIENT_e type, const std::vector<cindex_t> &normal_cindex, const std::vector<cindex2_t> &pml_cindex, const std::vector<index_t> &pml_index) = 0;

		// 係数リストを格納する
		virtual void storeCoefficientList(const std::vector<rvec2> &coef2_list, const std::vector<rvec3> &coef3_list) = 0;

		// 給電と観測を行う
		virtual void feedAndMeasure(void) = 0;

		// 電界を計算する
		virtual void calcEField(void) = 0;

		// 磁界を計算する
		virtual void calcHField(void) = 0;




	protected:
		/*// シミュレーション開始前の処理を行う
		virtual bool presimulation(const DSolverMaster &master, const DSituation &situation) = 0;

		// シミュレーション終了時の処理を行う
		virtual bool postsimulation(void) = 0;

		

		

		// 端部の電界を取得する
		virtual void getBoundaryEField(DIR_e dir, real *ex, real *ey, real *ez) = 0;

		// 端部の電界を設定する
		virtual void setBoundaryEField(DIR_e dir, const real *ex, const real *ey, const real *ez) = 0;

		// 端部の磁界を取得する
		virtual void getBoundaryHField(DIR_e dir, real *hx, real *hy, real *hz) = 0;

		// 端部の磁界を設定する
		virtual void setBoundaryHField(DIR_e dir, const real *hx, const real *hy, const real *hz) = 0;*/
	
	public:
		



	};
}
