#include "FFSituation.h"
#include "FFConst.h"
#include <algorithm>
#include <string>
#include <sstream>



namespace MUFDTD{
	// コンストラクタ
	FFSituation::FFSituation(const FFGrid &grid_x, const FFGrid &grid_y, const FFGrid &grid_z, const BC_t &bc, index_t offset, index_t size)
		: m_GlobalSize(grid_x.count(), grid_y.count(), grid_z.count())
		, m_OffsetZ(offset), m_SizeZ(size)
		, m_GridX(grid_x), m_GridY(grid_y), m_GridZ(grid_z)
		, m_Space(nullptr)
		, m_BC(bc)
	{
		// 領域のサイズをチェックする
		if (grid_z.count() < (offset + size)) throw;
		if (grid_x.count() <= (2 * bc.pmlL)) throw;
		if (grid_y.count() <= (2 * bc.pmlL)) throw;
		if (grid_z.count() <= (2 * bc.pmlL)) throw;

		// 空間配置情報を格納するメモリーを確保する
		m_Space = new MUSpace(grid_x.count(), grid_y.count(), size);

		// 周期境界条件のループ元座標を設定する
		if ((bc.z == BoundaryCondition::Periodic) && ((offset + size) == grid_z.count())){
			// ローカル領域は周期境界条件の正端
			m_LoopZ = 0;
		}
		else if ((offset + size) < grid_z.count()){
			// ローカル領域の正端には別の領域が接続される
			m_LoopZ = offset + size;
		}
		else{
			// ローカル領域の正端には何もない
			m_LoopZ = ~(index_t)0;
		}

		// グリッドの付随情報を計算する
		m_GridX.precompute(bc.x == BoundaryCondition::Periodic);
		m_GridY.precompute(bc.y == BoundaryCondition::Periodic);
		m_GridZ.precompute(bc.z == BoundaryCondition::Periodic);
	}

	// デストラクタ
	FFSituation::~FFSituation(){
		

		delete m_Space;

	}

	// 最適なタイムステップを計算する
	double FFSituation::calcTimestep(void) const{
		// クーラン条件より最適なタイムステップを計算する
		double min_dx_2 = pow(m_GridX.minimumWidth(), 2);
		double min_dy_2 = pow(m_GridY.minimumWidth(), 2);
		double min_dz_2 = pow(m_GridZ.minimumWidth(), 2);
		return 1.0 / (C * sqrt(1.0 / min_dx_2 + 1.0 / min_dy_2 + 1.0 / min_dz_2));
	}

	// 境界条件を取得する
	BoundaryCondition FFSituation::getBC(AXIS_e axis) const{
		if (axis == AXIS_X) return m_BC.x;
		else if (axis == AXIS_Y) return m_BC.y;
		else return m_BC.z;
	}

	// X方向に媒質を配置する
	void FFSituation::placeMediumX(matid_t matid, const index3_t &gpos, index_t count){
		if (matid == MATID_PEC){
			// PEC
			placePECBoxX(gpos, count);
		}
		else{
			// 媒質
			index3_t lpos(gpos.x, gpos.y, gpos.z - m_OffsetZ);

			// 領域のX,Y方向の範囲外への配置を除外する
			if (m_GlobalSize.x < (lpos.x + count)) throw;
			if (m_GlobalSize.y <= lpos.y) throw;

			// Z座標がループ元座標に一致した場合は領域の正端へ配置をコピーする
			if (gpos.z == m_LoopZ){
				m_Space->placeMedium(matid, index3_t(lpos.x, lpos.y, m_SizeZ), count);
			}

			// 領域のZ方向の範囲外への配置を除外する
			if (m_SizeZ <= lpos.z) throw;

			// 配置する
			m_Space->placeMedium(matid, lpos, count);
		}
	}

	// X方向にX方向のPECワイヤーを配置する
	void FFSituation::placePECXX(const index3_t &gpos, index_t count){
		index3_t lpos(gpos.x, gpos.y, gpos.z - m_OffsetZ);
		
		// 領域のX方向の範囲外への配置を除外する
		if (m_GlobalSize.x < (lpos.x + count)) throw;
		if (m_GlobalSize.y < lpos.y) throw;

		// Z座標がループ元座標に一致した場合は領域の正端へ配置をコピーする
		if (gpos.z == m_LoopZ){
			m_Space->placePECX(index3_t(lpos.x, lpos.y, m_SizeZ), count);
		}

		// 領域のZ方向の範囲外への配置を除外する
		if (m_SizeZ <= lpos.z) throw;

		m_Space->placePECX(lpos, count);
	}

	// X方向にY方向のPECワイヤーを配置する
	void FFSituation::placePECYX(const index3_t &gpos, index_t count){
		index3_t lpos(gpos.x, gpos.y, gpos.z - m_OffsetZ);

		// 領域のX,Y方向の範囲外への配置を除外する
		if ((m_GlobalSize.x + 1) < (lpos.x + count)) throw;
		if (m_GlobalSize.y <= lpos.y) throw;

		// Z座標がループ元座標に一致した場合は領域の正端へ配置をコピーする
		if (gpos.z == m_LoopZ){
			m_Space->placePECY(index3_t(lpos.x, lpos.y, m_SizeZ), count);
		}
		
		// 領域のZ方向の範囲外への配置を除外する
		if (m_SizeZ <= lpos.z) throw;

		m_Space->placePECY(lpos, count);
	}

	// X方向にZ方向のPECワイヤーを配置する
	void FFSituation::placePECZX(const index3_t &gpos, index_t count){
		index3_t lpos(gpos.x, gpos.y, gpos.z - m_OffsetZ);

		// 領域のX,Y,Z方向の範囲外への配置を除外する
		if ((m_GlobalSize.x + 1) < (lpos.x + count)) throw;
		if (m_GlobalSize.y < lpos.y) throw;
		if (m_SizeZ <= lpos.z) throw;
		
		m_Space->placePECZ(lpos, count);
	}

	// X方向にPECボックスを配置する
	void FFSituation::placePECBoxX(const index3_t &gpos, index_t count){
		placePECXX(gpos, count);
		placePECXX(index3_t(gpos.x, gpos.y + 1, gpos.z), count);
		placePECXX(index3_t(gpos.x, gpos.y, gpos.z + 1), count);
		placePECXX(index3_t(gpos.x, gpos.y + 1, gpos.z + 1), count);
		placePECYX(gpos, count + 1);
		placePECYX(index3_t(gpos.x, gpos.y, gpos.z + 1), count + 1);
		placePECZX(gpos, count + 1);
		placePECZX(index3_t(gpos.x, gpos.y + 1, gpos.z), count + 1);
	}

	// ボクセルデータを展開する
	void FFSituation::loadVoxelData(const index3_t &offset, FFVoxelReader *voxel_reader, VoxelOPs opecode){













	}




}


