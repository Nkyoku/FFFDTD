#include "FFSituation.h"
#include "FFConst.h"
#include <algorithm>
//#include <string>



namespace MUFDTD{
	// コンストラクタ
	FFSituation::FFSituation(void)
		: m_Size(0, 0, 0)
		, m_LocalOffsetZ(0), m_LocalSizeZ(0)
		, m_GridX(), m_GridY(), m_GridZ()
		, m_Volume()
		, m_BC()
	{

	}

	// デストラクタ
	FFSituation::~FFSituation(){
		// 材質リストを削除する
		initializeMaterialList(0);
		

	}

#pragma region 初期化関連のメソッド
	// グリッドを設定する
	void FFSituation::setGrids(const FFGrid &grid_x, const FFGrid &grid_y, const FFGrid &grid_z, const BC_t &bc){
		// グリッド情報と境界条件をコピーする
		m_Size.x = grid_x.count();
		m_Size.y = grid_y.count();
		m_Size.z = grid_z.count();
		m_GridX = grid_x;
		m_GridY = grid_y;
		m_GridZ = grid_z;
		m_BC = bc;
		if ((m_Size.x <= (m_BC.pmlL.x * 2)) || (m_Size.y <= (m_BC.pmlL.y * 2)) || ((m_Size.z <= m_BC.pmlL.z * 2))){
			// 空間サイズが境界条件の層数より小さい
			throw;
		}

		// グリッドの付随情報を計算する
		m_GridX.precompute(bc.x == BoundaryCondition::Periodic);
		m_GridY.precompute(bc.y == BoundaryCondition::Periodic);
		m_GridZ.precompute(bc.z == BoundaryCondition::Periodic);
	}

	// 処理の分割を設定する
	void FFSituation::setDivision(index_t offset, index_t size){
		// 領域のサイズをチェックする
		if (m_GridZ.count() < (offset + size)){
			throw;
		}

		// 接続元の座標を設定する
		if ((m_BC.z == BoundaryCondition::Periodic) && ((offset + size) == m_GridZ.count())){
			// ローカル領域は周期境界条件の正端
			m_ConnectionZ = 0;
		}
		else if ((offset + size) < m_GridZ.count()){
			// ローカル領域の正端には別の領域が接続される
			m_ConnectionZ = offset + size;
		}
		else{
			// ローカル領域の正端には何もない
			m_ConnectionZ = ~(index_t)0;
		}

		m_LocalOffsetZ = offset;
		m_LocalSizeZ = size;
	}

	// ボリュームデータを作成する
	void FFSituation::createVolumeData(void){
		// ボリュームを作成する
		m_Volume = FFVolumeData(m_Size.x, m_Size.y, m_Size.z);
		m_PECX = FFBitVolumeData(m_Size.x, m_Size.y, m_Size.z);
		m_PECY = FFBitVolumeData(m_Size.x, m_Size.y, m_Size.z);
		m_PECZ = FFBitVolumeData(m_Size.x, m_Size.y, m_Size.z);

		// スライスを作成する
		m_Volume.createSlices(m_LocalOffsetZ, m_LocalOffsetZ + m_LocalSizeZ - 1, MATID_PEC);
		m_PECX.createSlices(m_LocalOffsetZ, m_LocalOffsetZ + m_LocalSizeZ - 1, false);
		m_PECY.createSlices(m_LocalOffsetZ, m_LocalOffsetZ + m_LocalSizeZ - 1, false);
		m_PECZ.createSlices(m_LocalOffsetZ, m_LocalOffsetZ + m_LocalSizeZ - 1, false);
		if (m_ConnectionZ < m_Size.z){
			m_Volume.createSlices(m_ConnectionZ, m_ConnectionZ, MATID_PEC);
			//m_PECX.createSlices(m_ConnectionZ, m_ConnectionZ, false);
			//m_PECY.createSlices(m_ConnectionZ, m_ConnectionZ, false);
		}
	}
#pragma endregion

#pragma region 材質関連のメソッド
	// 材質リストを初期化する
	void FFSituation::initializeMaterialList(size_t count){
		for (FFMaterial *&mat : m_MaterialList){
			delete mat;
			mat = nullptr;
		}
		if (((size_t)MAX_MATID + 1) < count){
			// 材質データの数が上限に達している
			throw;
		}
		m_MaterialList.resize(count);

		// PECの材質IDを登録する
		if (MATID_PEC < count){
			m_MaterialList[MATID_PEC] = new FFMaterial();
		}
	}

	// 新しく材質データを登録する
	matid_t FFSituation::registerMaterial(FFMaterial *mat){
		if ((MAX_MATID + 1) <= m_MaterialList.size()){
			// 材質データの数が上限に達している
			throw;
		}
		m_MaterialList.push_back(mat);
		return (matid_t)(m_MaterialList.size() - 1);
	}

	// 指定した材質IDで材質データを登録する
	void FFSituation::registerMaterial(matid_t matid, FFMaterial *mat){
		if (m_MaterialList.size() <= matid){
			// 範囲外の材質IDが指定された場合は材質リストを拡張する
			if (MAX_MATID < matid){
				// 指定された材質IDが大きすぎる
				throw;
			}
			m_MaterialList.resize((size_t)matid + 1, nullptr);
		}else if (m_MaterialList[matid] != nullptr){
			// 同じ材質IDの材質データがすでに存在する
			throw;
		}
		m_MaterialList[matid] = mat;
	}

	// 材質リストのすべての材質の物性値が指定されているか調べる
	bool FFSituation::isMaterialListFilled(void){
		for (FFMaterial *mat : m_MaterialList){
			if (mat == nullptr){
				return false;
			}
		}
		return true;
	}

	// 指定した材質IDの材質データを取得する
	const FFMaterial* FFSituation::getMaterialByID(matid_t matid){
		if (MAX_MATID < matid){
			// 無効な材質IDを参照しようとしている
			throw;
		}
		if (m_MaterialList.size() <= matid){
			// 材質データは存在しない
			return nullptr;
		}
		return m_MaterialList[matid];
	}
#pragma endregion

#pragma region シミュレーション環境の情報を取得するメソッド
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
		if (axis == AXIS_X){
			return m_BC.x;
		}
		else if (axis == AXIS_Y){
			return m_BC.y;
		}
		else{
			return m_BC.z;
		}
	}
#pragma endregion

#pragma region シミュレーション環境を作成するメソッド
	// 指定した材質IDの直方体を配置する
	bool FFSituation::placeCuboid(matid_t matid, const index3_t &pos1, const index3_t &pos2){
		// 座標の順序を正す
		index_t ix1 = (pos1.x < pos2.x) ? pos1.x : pos2.x;
		index_t ix2 = (pos1.x < pos2.x) ? pos2.x : pos1.x;
		index_t iy1 = (pos1.y < pos2.y) ? pos1.y : pos2.y;
		index_t iy2 = (pos1.y < pos2.y) ? pos2.y : pos1.y;
		index_t iz1 = (pos1.z < pos2.z) ? pos1.z : pos2.z;
		index_t iz2 = (pos1.z < pos2.z) ? pos2.z : pos1.z;

		// 直方体が領域内にあるかチェックする
		if ((m_Size.x <= ix1) || (m_Size.y <= iy1) || (m_Size.z <= iz1)){
			return false;
		}
		
		// 領域外へ飛び出た部分をカットする
		if (m_Size.x < ix2) ix2 = m_Size.x;
		if (m_Size.y < iy2) iy2 = m_Size.y;
		if (m_Size.z < iz2) iz2 = m_Size.z;

		// 指定された材質IDを設定する
		for (index_t iz = iz1; iz < iz2; iz++){
			FFSliceData *slice = m_Volume.getSlice(iz);
			if (slice != nullptr){
				for (index_t iy = iy1; iy < iy2; iy++){
					for (index_t ix = ix1; ix < ix2; ix++){
						slice->setPoint(ix, iy, matid);
					}
				}
			}
		}

		return true;
	}

	// PECワイヤーの直方体を配置する
	bool FFSituation::placePECCuboid(const index3_t &pos1, const index3_t &pos2){
		// 座標の順序を正す
		index_t ix1 = (pos1.x < pos2.x) ? pos1.x : pos2.x;
		index_t ix2 = (pos1.x < pos2.x) ? pos2.x : pos1.x;
		index_t iy1 = (pos1.y < pos2.y) ? pos1.y : pos2.y;
		index_t iy2 = (pos1.y < pos2.y) ? pos2.y : pos1.y;
		index_t iz1 = (pos1.z < pos2.z) ? pos1.z : pos2.z;
		index_t iz2 = (pos1.z < pos2.z) ? pos2.z : pos1.z;

		// 直方体が領域内にあるかチェックする
		if ((m_Size.x < ix1) || (m_Size.y < iy1) || (m_Size.z < iz1)){
			return false;
		}

		// 領域外へ飛び出た部分をカットする
		if (m_Size.x < ix2) ix2 = m_Size.x;
		if (m_Size.y < iy2) iy2 = m_Size.y;
		if (m_Size.z < iz2) iz2 = m_Size.z;

		// PECワイヤーを設定する
		for (index_t iz = iz1; iz <= iz2; iz++){
			FFBitSliceData *slice = m_PECX.getSliceRepeat(iz);
			if (slice != nullptr){
				for (index_t iy = iy1; iy <= iy2; iy++){
					for (index_t ix = ix1; ix < ix2; ix++){
						slice->setPointRepeat(ix, iy, true);
					}
				}
			}
		}
		for (index_t iz = iz1; iz <= iz2; iz++){
			FFBitSliceData *slice = m_PECY.getSliceRepeat(iz);
			if (slice != nullptr){
				for (index_t iy = iy1; iy < iy2; iy++){
					for (index_t ix = ix1; ix <= ix2; ix++){
						slice->setPointRepeat(ix, iy, true);
					}
				}
			}
		}
		for (index_t iz = iz1; iz < iz2; iz++){
			FFBitSliceData *slice = m_PECZ.getSliceRepeat(iz);
			if (slice != nullptr){
				for (index_t iy = iy1; iy <= iy2; iy++){
					for (index_t ix = ix1; ix <= ix2; ix++){
						slice->setPointRepeat(ix, iy, true);
					}
				}
			}
		}

		return true;
	}

	/*// PECデータをストリームから読み込む
	void FFSituation::loadPECData(FFIStream &stream){
	// ボリュームデータを読み込む
	m_PEC.loadFromIStream(stream, m_LocalOffsetZ, m_LocalOffsetZ + m_LocalSizeZ - 1, m_ConnectionZ == 0);
	index3_t size = m_PEC.getSize();
	if (size != m_Size){
	throw;
	}
	if ((PECFLAG_X | PECFLAG_Y | PECFLAG_Z) < m_PEC.getMaximumMaterialID()){
	throw;
	}
	}

	// ボリュームデータを読み込む
	void FFSituation::loadVolumeData(FFIStream &stream){
	// ボリュームデータを読み込む
	m_Volume.loadFromIStream(stream, m_LocalOffsetZ, m_LocalOffsetZ + m_LocalSizeZ - 1, m_ConnectionZ == 0);
	index3_t size = m_Volume.getSize();
	if (size != m_Size){
	throw;
	}

	// 材質リストを初期化する
	for (auto mat : m_MaterialList){
	delete mat;
	mat = nullptr;
	}
	m_MaterialList.resize((size_t)m_Volume.getMaximumMaterialID() + 1);

	// PECの材質IDを作成する
	m_MaterialList[0] = new FFMaterial();
	}

	// ボリュームデータとPECデータを現在のグリッド設定から作成する
	void FFSituation::createVolumeAndPECDataFromGrid(void){
	m_Volume = FFVolumeData(m_Size.x, m_Size.y, m_Size.z, MATID_PEC);
	m_PEC = FFVolumeData(m_Size.x, m_Size.y, m_Size.z, 0);

	// 材質リストを初期化する
	for (auto mat : m_MaterialList){
	delete mat;
	mat = nullptr;
	}
	m_MaterialList.resize(1);

	// PECの材質IDを作成する
	m_MaterialList[0] = new FFMaterial();
	}*/
#pragma endregion

#pragma region 係数を計算するメソッド
	// Exに作用する物性値を取得する
	bool FFSituation::getMaterialEx(const index3_t &pos, FFMaterial *material) const{
		const FFMaterial *mat1 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z)];
		const FFMaterial *mat2 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y - 1, pos.z)];
		const FFMaterial *mat3 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z - 1)];
		const FFMaterial *mat4 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y - 1, pos.z - 1)];
		if ((mat1 == nullptr) || (mat2 == nullptr) || (mat3 == nullptr) || (mat4 == nullptr)){
			throw;
		}
		*material = FFMaterial(
			(mat1->eps_r() + mat2->eps_r() + mat3->eps_r() + mat4->eps_r()) * 0.25,
			(mat1->sigma() + mat2->sigma() + mat3->sigma() + mat4->sigma()) * 0.25,
			(mat1->mu_r() + mat2->mu_r() + mat3->mu_r() + mat4->mu_r()) * 0.25);
		return m_PECX.getPointRepeat(pos.x, pos.y, pos.z);
	}

	// Eyに作用する物性値を取得する
	bool FFSituation::getMaterialEy(const index3_t &pos, FFMaterial *material) const{
		const FFMaterial *mat1 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z)];
		const FFMaterial *mat2 = m_MaterialList[m_Volume.getPointRepeat(pos.x - 1, pos.y, pos.z)];
		const FFMaterial *mat3 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z - 1)];
		const FFMaterial *mat4 = m_MaterialList[m_Volume.getPointRepeat(pos.x - 1, pos.y, pos.z - 1)];
		if ((mat1 == nullptr) || (mat2 == nullptr) || (mat3 == nullptr) || (mat4 == nullptr)){
			throw;
		}
		*material = FFMaterial(
			(mat1->eps_r() + mat2->eps_r() + mat3->eps_r() + mat4->eps_r()) * 0.25,
			(mat1->sigma() + mat2->sigma() + mat3->sigma() + mat4->sigma()) * 0.25,
			(mat1->mu_r() + mat2->mu_r() + mat3->mu_r() + mat4->mu_r()) * 0.25);
		return m_PECY.getPointRepeat(pos.x, pos.y, pos.z);
	}

	// Ezに作用する物性値を取得する
	bool FFSituation::getMaterialEz(const index3_t &pos, FFMaterial *material) const{
		const FFMaterial *mat1 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z)];
		const FFMaterial *mat2 = m_MaterialList[m_Volume.getPointRepeat(pos.x - 1, pos.y, pos.z)];
		const FFMaterial *mat3 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y - 1, pos.z)];
		const FFMaterial *mat4 = m_MaterialList[m_Volume.getPointRepeat(pos.x - 1, pos.y - 1, pos.z)];
		if ((mat1 == nullptr) || (mat2 == nullptr) || (mat3 == nullptr) || (mat4 == nullptr)){
			throw;
		}
		*material = FFMaterial(
			(mat1->eps_r() + mat2->eps_r() + mat3->eps_r() + mat4->eps_r()) * 0.25,
			(mat1->sigma() + mat2->sigma() + mat3->sigma() + mat4->sigma()) * 0.25,
			(mat1->mu_r() + mat2->mu_r() + mat3->mu_r() + mat4->mu_r()) * 0.25);
		return m_PECZ.getPointRepeat(pos.x, pos.y, pos.z);
	}

	// Hxに作用する物性値を取得する
	void FFSituation::getMaterialHx(const index3_t &pos, FFMaterial *material) const{
		const FFMaterial *mat1 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z)];
		const FFMaterial *mat2 = m_MaterialList[m_Volume.getPointRepeat(pos.x - 1, pos.y, pos.z)];
		if ((mat1 == nullptr) || (mat2 == nullptr)){
			throw;
		}
		*material = FFMaterial(
			(mat1->eps_r() + mat2->eps_r()) * 0.5,
			(mat1->sigma() + mat2->sigma()) * 0.5,
			(mat1->mu_r() + mat2->mu_r()) * 0.5);
	}

	// Hyに作用する物性値を取得する
	void FFSituation::getMaterialHy(const index3_t &pos, FFMaterial *material) const{
		const FFMaterial *mat1 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z)];
		const FFMaterial *mat2 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y - 1, pos.z)];
		if ((mat1 == nullptr) || (mat2 == nullptr)){
			throw;
		}
		*material = FFMaterial(
			(mat1->eps_r() + mat2->eps_r()) * 0.5,
			(mat1->sigma() + mat2->sigma()) * 0.5,
			(mat1->mu_r() + mat2->mu_r()) * 0.5);
	}

	// Hzに作用する物性値を取得する
	void FFSituation::getMaterialHz(const index3_t &pos, FFMaterial *material) const{
		const FFMaterial *mat1 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z)];
		const FFMaterial *mat2 = m_MaterialList[m_Volume.getPointRepeat(pos.x, pos.y, pos.z - 1)];
		if ((mat1 == nullptr) || (mat2 == nullptr)){
			throw;
		}
		*material = FFMaterial(
			(mat1->eps_r() + mat2->eps_r()) * 0.5,
			(mat1->sigma() + mat2->sigma()) * 0.5,
			(mat1->mu_r() + mat2->mu_r()) * 0.5);
	}
#pragma endregion

	


}


