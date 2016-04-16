#pragma once

#include "FFGrid.h"
#include "FFSpace.h"
//#include "Circuit/FFCircuit.h"
#include "Format/FFVoxelReader.h"
//#include <vector>



namespace MUFDTD{
	// 境界条件を格納する構造体
	struct BC_t{
		BoundaryCondition x, y, z;
		index_t pmlL;
		double pmlM;
		double pmlR0;
	};

	

	// シミュレーション環境を作成するクラス
	class FFSituation{
		/*** 定数 ***/
	private:
		// 真空の物質ID
		static const matid_t MATID_VACUUM = MUSpace::MATID_VACUUM;

		// PECの媒質ID
		static const matid_t MATID_PEC = MUSpace::MATID_PEC;



		/*** メンバー変数 ***/
	private:
		// グローバル領域のサイズ
		index3_t m_GlobalSize;

		// ローカル領域のオフセット
		//index3_t m_Offset;
		index_t m_OffsetZ;
		
		// ローカル領域のサイズ
		//index3_t m_Size;
		index_t m_SizeZ;

		// ループ元の座標
		//index3_t m_LoopPos;
		index_t m_LoopZ;

		// グリッド情報
		FFGrid m_GridX, m_GridY, m_GridZ;

		// 媒質の空間配置情報
		MUSpace *m_Space;

		// 境界条件
		BC_t m_BC;



		/*** メソッド ***/
	public:
		// コンストラクタ
		FFSituation(const FFGrid &grid_x, const FFGrid &grid_y, const FFGrid &grid_z, const BC_t &bc, index_t offset, index_t size);

		// デストラクタ
		~FFSituation();

	public:
		// 最適なタイムステップを計算する
		double calcTimestep(void) const;

		// グローバル領域の大きさを取得する
		const index3_t getGlobalSize(void) const{
			return m_GlobalSize;
		}

		// ローカル領域の大きさを取得する
		index_t getLocalSize(void) const{
			return m_SizeZ;
		}

		// ローカル領域のオフセットを取得する
		index_t getLocalOffset(void) const{
			return m_OffsetZ;
		}

		// 境界条件を取得する
		BoundaryCondition getBC(AXIS_e axis) const;

		// PML吸収境界条件の層数を取得する
		int getPmlL(void) const{
			return m_BC.pmlL;
		}

		// PML吸収境界条件の次数を取得する
		double getPmlM(void) const{
			return m_BC.pmlM;
		}

		// PML吸収境界条件の反射係数を取得する
		double getPmlR0(void) const{
			return m_BC.pmlR0;
		}

	private:
		// X方向に媒質を配置する
		void placeMediumX(matid_t matid, const index3_t &gpos, index_t count);

		// X方向にX方向のPECワイヤーを配置する
		void placePECXX(const index3_t &gpos, index_t count);

		// X方向にY方向のPECワイヤーを配置する
		void placePECYX(const index3_t &gpos, index_t count);

		// X方向にZ方向のPECワイヤーを配置する
		void placePECZX(const index3_t &gpos, index_t count);

		// X方向にPECボックスを配置する
		void placePECBoxX(const index3_t &gpos, index_t count);

	public:
		// ボクセルデータを展開する
		void loadVoxelData(const index3_t &offset, FFVoxelReader *voxel_reader, VoxelOPs opecode = VoxelOPs::Add);





	};
}
