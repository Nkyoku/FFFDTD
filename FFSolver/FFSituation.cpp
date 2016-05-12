#include "FFSituation.h"
#include "FFConst.h"
#include <algorithm>
#include <iterator>
#include <mpi.h>



namespace FFFDTD{
	// コンストラクタ
	FFSituation::FFSituation(void)
		: m_Timestep(0.0)
		, m_Size(0, 0, 0)
		, m_LocalOffsetZ(0), m_LocalSizeZ(0)
		, m_GridX(), m_GridY(), m_GridZ()
		, m_BC()
		, m_Volume(), m_PECX(), m_PECY(), m_PECZ()
		, m_MaterialList()
		, m_PortList()
		, m_TDProbeList(), m_FDProbeList()
		, m_Solver(nullptr)
		, m_NT(0), m_IT(0)
		, m_FreqList()
	{

	}

	// デストラクタ
	FFSituation::~FFSituation(){
		// 材質リストを削除する
		initializeMaterialList(0);
		
		// ポートリストを削除する
		for (FFPort *port : m_PortList){
			delete port;
		}

		// ソルバーを削除する
		configureSolver(nullptr, 0.0, 0, std::vector<double>());
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
		if (isConnectedZ()){
			m_Volume.createSlices(m_ConnectionZ, m_ConnectionZ, MATID_PEC);
			m_PECX.createSlices(m_ConnectionZ, m_ConnectionZ, false);
			m_PECY.createSlices(m_ConnectionZ, m_ConnectionZ, false);
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
	BoundaryCondition FFSituation::getBC(Axis axis) const{
		if (axis == Axis::X){
			return m_BC.x;
		}
		else if (axis == Axis::Y){
			return m_BC.y;
		}
		else{
			return m_BC.z;
		}
	}

	// ポートのリストを取得する
	std::vector<const FFPort*> FFSituation::getPortList(void) const{
		std::vector<const FFPort*> result(m_PortList.size(), nullptr);
		std::copy(m_PortList.begin(), m_PortList.end(), std::back_inserter(result));
		return result;
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

	/*// 観測点を配置する
	oindex_t FFSituation::placeProbePoint(const FFPointObject &object){
		index3_t pos = object.getPos();
		DIR_e dir = object.getDir();
		if ((dir == X_PLUS) || (dir == X_MINUS)){
			if ((m_Size.x <= pos.x) || (m_Size.y < pos.y) || (m_Size.z < pos.z)){
				throw;
			}
			if (pos.y == 0){
				pos.y = m_Size.y;
			}
			if (pos.z == 0){
				pos.z = m_Size.z;
			}
		}
		else if ((dir == Y_PLUS) || (dir == Y_MINUS)){
			if ((m_Size.x < pos.x) || (m_Size.y <= pos.y) || (m_Size.z < pos.z)){
				throw;
			}
			if (pos.x == 0){
				pos.x = m_Size.x;
			}
			if (pos.z == 0){
				pos.z = m_Size.z;
			}
		}
		else if ((dir == Z_PLUS) || (dir == Z_MINUS)){
			if ((m_Size.x < pos.x) || (m_Size.y < pos.y) || (m_Size.z <= pos.z)){
				throw;
			}
			if (pos.x == 0){
				pos.x = m_Size.x;
			}
			if (pos.y == 0){
				pos.y = m_Size.y;
			}
		}
		else{
			throw;
		}
		m_ProbePointList.push_back(FFPointObject(pos, dir));
		return (oindex_t)(m_ProbePointList.size() - 1);
	}

	// 観測面を配置する
	oindex_t FFSituation::placeProbePlane(const FFPointObject &object){
		index3_t pos = object.getPos();
		DIR_e dir = object.getDir();
		if ((dir == X_PLUS) || (dir == X_MINUS)){
			if ((m_Size.x < pos.x) || (m_Size.y <= pos.y) || (m_Size.z <= pos.z)){
				throw;
			}
			if (pos.x == 0){
				pos.x = m_Size.x;
			}
		}
		else if ((dir == Y_PLUS) || (dir == Y_MINUS)){
			if ((m_Size.x <= pos.x) || (m_Size.y < pos.y) || (m_Size.z <= pos.z)){
				throw;
			}
			if (pos.y == 0){
				pos.y = m_Size.y;
			}
		}
		else if ((dir == Z_PLUS) || (dir == Z_MINUS)){
			if ((m_Size.x <= pos.x) || (m_Size.y <= pos.y) || (m_Size.z < pos.z)){
				throw;
			}
			if (pos.z == 0){
				pos.z = m_Size.z;
			}
		}
		else{
			throw;
		}
		m_ProbePlaneList.push_back(FFPointObject(pos, dir));
		return (oindex_t)(m_ProbePlaneList.size() - 1);
	}*/

	// ポートを配置する
	oindex_t FFSituation::placePort(const index3_t &pos_, DIR_e dir, FFCircuit *circuit){
		// ポートが計算領域に含まれるか調べる
		bool out_of_bounding, out_of_local;
		index3_t pos;
		if ((dir == X_PLUS) || (dir == X_MINUS)){
			pos.x = pos_.x;
			pos.y = (pos_.y != 0) ? pos_.y : m_Size.y;
			pos.z = (pos_.z != 0) ? pos_.z : m_Size.z;
			out_of_bounding  = (m_Size.x <= pos.x);
			out_of_bounding |= isConnectedY() ? (m_Size.y < pos.y) : (m_Size.y <= pos.y);
			out_of_bounding |= isConnectedZ() ? (m_Size.z < pos.z) : (m_Size.z <= pos.z);
			out_of_local = (pos.z <= m_LocalOffsetZ) || (isConnectedZ() ? ((m_LocalOffsetZ + m_LocalSizeZ) < pos.z) : ((m_LocalOffsetZ + m_LocalSizeZ) <= pos.z));
		}
		else if ((dir == Y_PLUS) || (dir == Y_MINUS)){
			pos.x = (pos_.x != 0) ? pos_.x : m_Size.x;
			pos.y = pos_.y;
			pos.z = (pos_.z != 0) ? pos_.z : m_Size.z;
			out_of_bounding  = isConnectedX() ? (m_Size.x < pos.x) : (m_Size.x <= pos.x);
			out_of_bounding |= (m_Size.y <= pos.y);
			out_of_bounding |= isConnectedZ() ? (m_Size.z < pos.z) : (m_Size.z <= pos.z);
			out_of_local = (pos.z <= m_LocalOffsetZ) || (isConnectedZ() ? ((m_LocalOffsetZ + m_LocalSizeZ) < pos.z) : ((m_LocalOffsetZ + m_LocalSizeZ) <= pos.z));
		}
		else if ((dir == Z_PLUS) || (dir == Z_MINUS)){
			pos.x = (pos_.x != 0) ? pos_.x : m_Size.x;
			pos.y = (pos_.y != 0) ? pos_.y : m_Size.y;
			pos.z = pos_.z;
			out_of_bounding  = isConnectedX() ? (m_Size.x < pos.x) : (m_Size.x <= pos.x);
			out_of_bounding |= isConnectedY() ? (m_Size.y < pos.y) : (m_Size.y <= pos.y);
			out_of_bounding |= (m_Size.z <= pos.z);
			out_of_local = (pos.z < m_LocalOffsetZ) || ((m_LocalOffsetZ + m_LocalSizeZ) <= pos.z);
		}
		else{
			throw;
		}
		if (out_of_bounding == true){
			throw;
		}

		if (out_of_local == false){
			// ポートを作成する
			FFPort *port = new FFPort(circuit);
			double sign = ((dir == X_PLUS) || (dir == Y_PLUS) || (dir == Z_PLUS)) ? 1.0 : -1.0;
			if ((dir == X_PLUS) || (dir == X_MINUS)){
				port->attachEProbe(placeProbe(pos, EMType::Ex, ProbeType::TD), sign * m_GridX.iwidth(pos.x));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z), EMType::Hz, ProbeType::TD), sign * m_GridZ.mwidth(pos.z));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y - 1, pos.z), EMType::Hz, ProbeType::TD), -sign * m_GridZ.mwidth(pos.z));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z), EMType::Hy, ProbeType::TD), -sign * m_GridY.mwidth(pos.y));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z - 1), EMType::Hy, ProbeType::TD), sign * m_GridY.mwidth(pos.y));
			}
			else if ((dir == Y_PLUS) || (dir == Y_MINUS)){
				port->attachEProbe(placeProbe(pos, EMType::Ey, ProbeType::TD), sign * m_GridY.iwidth(pos.y));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z), EMType::Hx, ProbeType::TD), sign * m_GridX.mwidth(pos.x));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z - 1), EMType::Hx, ProbeType::TD), -sign * m_GridX.mwidth(pos.x));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z), EMType::Hz, ProbeType::TD), -sign * m_GridZ.mwidth(pos.z));
				port->attachMProbe(placeProbe(index3_t(pos.x - 1, pos.y, pos.z), EMType::Hz, ProbeType::TD), sign * m_GridZ.mwidth(pos.z));
			}
			else if ((dir == Z_PLUS) || (dir == Z_MINUS)){
				port->attachEProbe(placeProbe(pos, EMType::Ez, ProbeType::TD), sign * m_GridZ.iwidth(pos.z));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z), EMType::Hy, ProbeType::TD), sign * m_GridY.mwidth(pos.y));
				port->attachMProbe(placeProbe(index3_t(pos.x - 1, pos.y, pos.z - 1), EMType::Hy, ProbeType::TD), -sign * m_GridY.mwidth(pos.y));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y, pos.z), EMType::Hx, ProbeType::TD), -sign * m_GridX.mwidth(pos.x));
				port->attachMProbe(placeProbe(index3_t(pos.x, pos.y - 1, pos.z), EMType::Hx, ProbeType::TD), sign * m_GridX.mwidth(pos.x));
			}
			m_PortList.push_back(port);
		}
		else{
			// ポートを欠番とする
			m_PortList.push_back(nullptr);
		}
		return (oindex_t)(m_PortList.size() - 1);
	}

	// プローブを配置する
	oindex_t FFSituation::placeProbe(const index3_t &pos, EMType em_type, ProbeType probe_type){
		// プローブの位置をチェックする
		switch (em_type){
		case EMType::Ex:
			if ((m_Size.x <= pos.x) || (m_Size.y < pos.y) || (pos.z < m_LocalOffsetZ) || ((m_LocalOffsetZ + m_LocalSizeZ) < pos.z)){
				throw;
			}
			break;

		case EMType::Ey:
			if ((m_Size.x < pos.x) || (m_Size.y <= pos.y) || (pos.z < m_LocalOffsetZ) || ((m_LocalOffsetZ + m_LocalSizeZ) < pos.z)){
				throw;
			}
			break;

		case EMType::Ez:
			if ((m_Size.x < pos.x) || (m_Size.y < pos.y) || (pos.z < m_LocalOffsetZ) || ((m_LocalOffsetZ + m_LocalSizeZ) <= pos.z)){
				throw;
			}
			break;

		case EMType::Hx:
			if ((m_Size.x < pos.x) || (m_Size.y <= pos.y) || (pos.z < m_LocalOffsetZ) || ((m_LocalOffsetZ + m_LocalSizeZ) <= pos.z)){
				throw;
			}
			break;

		case EMType::Hy:
			if ((m_Size.x <= pos.x) || (m_Size.y < pos.y) || (pos.z < m_LocalOffsetZ) || ((m_LocalOffsetZ + m_LocalSizeZ) <= pos.z)){
				throw;
			}
			break;

		case EMType::Hz:
			if ((m_Size.x <= pos.x) || (m_Size.y <= pos.y) || (pos.z < m_LocalOffsetZ) || ((m_LocalOffsetZ + m_LocalSizeZ) < pos.z)){
				throw;
			}
			break;
		};

		// リストに追加する
		Probe_t probe;
		probe.index = pos.x + (m_Size.x + 1) * (pos.y + (m_Size.y + 1) * (pos.z - m_LocalOffsetZ));
		probe.pos = pos;
		probe.type = em_type;
		if (probe_type == ProbeType::TD){
			m_TDProbeList.push_back(probe);
			return (oindex_t)(m_TDProbeList.size() - 1);
		}
		else if (probe_type == ProbeType::FD){
			m_FDProbeList.push_back(probe);
			return (oindex_t)(m_FDProbeList.size() - 1);
		}
		else{
			throw;
		}
	}
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

	
#pragma region ソルバーを操作するメソッド
	// ソルバーにシミュレーション環境を構成する
	void FFSituation::configureSolver(FFSolver *solver, double timestep, size_t max_iteration, const std::vector<double> &measure_freq){
		// ソルバーを上書きする
		delete m_Solver;
		m_Solver = solver;
		if (solver == nullptr){
			return;
		}

		// パラメータをコピーする
		if ((timestep <= 0.0) || (max_iteration <= 0)){
			throw;
		}
		m_Timestep = timestep;
		m_NT = max_iteration;
		m_IT = 0;
		m_FreqList = measure_freq;
		const double Dt = timestep;
		const size_t NF = measure_freq.size();

		// 解析空間の大きさを計算する
		index_t Mx = m_Size.x + (isConnectedX() ? 1 : 0);
		index_t My = m_Size.y + (isConnectedY() ? 1 : 0);
		index_t Mz = m_LocalSizeZ + (isConnectedZ() ? 1 : 0);
		index_t Nx = Mx + 1, Ny = My + 1, Nz = Mz + 1;
		index_t Lx = m_BC.pmlL.x, Ly = m_BC.pmlL.y, Lz = m_BC.pmlL.z;

		// ソルバーにメモリーを確保させる
		index_t z_start = (m_LocalOffsetZ < Lz) ? (Lz - m_LocalOffsetZ) : 0;
		index_t z_end = std::min(m_LocalSizeZ, m_Size.z - Lz - m_LocalOffsetZ);
		solver->initializeMemory(index3_t(Mx, My, Mz), index3_t(Lx, Ly, z_start), index3_t(Mx - 2 * Lx, My - 2 * Ly, z_end - z_start));

		// 係数リスト
		std::vector<rvec2> coef2_list(1, rvec2(0.0, 0.0));
		std::vector<rvec3> coef3_list(1, rvec3(0.0, 0.0, 0.0));
		const cindex_t pec_id = 0;

		// 2組係数を登録する関数
		auto registerCoef2 = [&](const rvec2 &coef) -> cindex_t{
			size_t index;
			for (index = 0; index < coef2_list.size(); index++){
				if (coef2_list[index] == coef){
					return static_cast<cindex_t>(index);
				}
			}
			coef2_list.push_back(coef);
			return static_cast<cindex_t>(index);
		};

		// 3組係数を登録する関数
		auto registerCoef3 = [&](const rvec3 &coef) -> cindex_t{
			size_t index;
			for (index = 0; index < coef3_list.size(); index++){
				if (coef3_list[index] == coef){
					return static_cast<cindex_t>(index);
				}
			}
			coef3_list.push_back(coef);
			return static_cast<cindex_t>(index);
		};

		// 導電率を計算する際の係数
		const double pml_m = getPmlM();
		const double sigma_max_k = -C * (pml_m + 1) * log(getPmlR0()) * 0.5;

		// PMLの最大導電率を計算する関数
		auto calcSigmaMax = [sigma_max_k](double eps, double width, int pml_l) -> double{
			return eps / (width * pml_l) * sigma_max_k;
		};

		// PMLの導電率を計算する関数
		auto calcSigma = [pml_m](double sigma_max, double i1, double i2, double i, int pml_l) -> double{
			if (i <= i1){
				return sigma_max * pow((i1 - i) / pml_l, pml_m);
			}
			else if (i2 <= i){
				return sigma_max * pow((i - i2) / pml_l, pml_m);
			}
			else{
				return 0.0;
			}
		};

		// 係数を計算する
#pragma omp parallel sections num_threads(1)
		{
			// Dx,Exに対する係数を計算する
#pragma omp section
			{
				std::vector<cindex_t> normal_cindex(Nx * Ny * Nz, pec_id);
				std::vector<cindex2_t> pml_cindex;
				std::vector<index_t> pml_index;
				for (index_t ilz = 1; ilz <= m_LocalSizeZ; ilz++){
					index_t iz = m_LocalOffsetZ + ilz;
					for (index_t iy = 1; iy < Ny; iy++){
						for (index_t ix = 0; ix < Mx; ix++){
							double dy = m_GridY.mwidth(iy);
							double dz = m_GridZ.mwidth(iz);
							FFMaterial mat;
							bool pec = getMaterialEx(index3_t(ix, iy, iz), &mat);
							bool inside_pml =
								(iz <= Lz) || (((Nz - 2 * Lz - 1) < iz) && (0 < Lz)) ||
								(iy <= Ly) || (((Ny - 2 * Ly - 1) < iy) && (0 < Ly)) ||
								(ix < Lx) || ((Mx - 2 * Lx) <= ix);
							if (inside_pml){
								double sigma_y_max = calcSigmaMax(mat.eps(), dy, Ly);
								double sigma_z_max = calcSigmaMax(mat.eps(), dz, Lz);
								double sigma_y = (0 < Ly) ? calcSigma(sigma_y_max, Ly, Ny - 2 * Ly - 1, iy, Ly) : 0.0;
								double sigma_z = (0 < Lz) ? calcSigma(sigma_z_max, Lz, Nz - 2 * Lz - 1, iz, Lz) : 0.0;
								pml_cindex.push_back(cindex2_t(
									registerCoef2(FFMaterial::calcDCoefPML(mat.eps_r(), sigma_y, Dt, dy)),
									registerCoef2(FFMaterial::calcDCoefPML(mat.eps_r(), sigma_z, Dt, dz))));
								pml_index.push_back(ix + Nx * (iy + Ny * ilz));
								normal_cindex[ix + Nx * (iy + Ny * ilz)] = pec ? pec_id : registerCoef2(mat.calcECoefPML(Dt));
							}
							else{
								normal_cindex[ix + Nx * (iy + Ny * ilz)] = pec ? pec_id : registerCoef3(mat.calcECoef(Dt, dy, dz));
							}
						}
					}
				}

#pragma omp critical
				m_Solver->storeCoefficientIndex(EMType::Ex, normal_cindex, pml_cindex, pml_index);
			}

			// Dy,Eyに対する係数を計算する
#pragma omp section
			{
				std::vector<cindex_t> normal_cindex(Nx * Ny * Nz, pec_id);
				std::vector<cindex2_t> pml_cindex;
				std::vector<index_t> pml_index;
				for (index_t ilz = 1; ilz <= m_LocalSizeZ; ilz++){
					index_t iz = m_LocalOffsetZ + ilz;
					for (index_t iy = 0; iy < My; iy++){
						for (index_t ix = 1; ix < Nx; ix++){
							double dz = m_GridZ.mwidth(iz);
							double dx = m_GridX.mwidth(ix);
							FFMaterial mat;
							bool pec = getMaterialEy(index3_t(ix, iy, iz), &mat);
							bool inside_pml =
								(iz <= Lz) || (((Nz - 2 * Lz - 1) < iz) && (0 < Lz)) ||
								(iy < Ly) || ((My - 2 * Ly) <= iy) ||
								(ix <= Lx) || (((Nx - 2 * Lx - 1) < ix) && (0 < Lx));
							if (inside_pml){
								double sigma_z_max = calcSigmaMax(mat.eps(), dz, Lz);
								double sigma_x_max = calcSigmaMax(mat.eps(), dx, Lx);
								double sigma_z = (0 < Lz) ? calcSigma(sigma_z_max, Lz, Nz - 2 * Lz - 1, iz, Lz) : 0.0;
								double sigma_x = (0 < Lx) ? calcSigma(sigma_x_max, Lx, Nx - 2 * Lx - 1, ix, Lx) : 0.0;
								pml_cindex.push_back(cindex2_t(
									registerCoef2(FFMaterial::calcDCoefPML(mat.eps_r(), sigma_z, Dt, dz)),
									registerCoef2(FFMaterial::calcDCoefPML(mat.eps_r(), sigma_x, Dt, dx))));
								pml_index.push_back(ix + Nx * (iy + Ny * ilz));
								normal_cindex[ix + Nx * (iy + Ny * ilz)] = pec ? pec_id : registerCoef2(mat.calcECoefPML(Dt));
							}
							else{
								normal_cindex[ix + Nx * (iy + Ny * ilz)] = pec ? pec_id : registerCoef3(mat.calcECoef(Dt, dz, dx));
							}
						}
					}
				}

#pragma omp critical
				m_Solver->storeCoefficientIndex(EMType::Ey, normal_cindex, pml_cindex, pml_index);
			}

			// Dz,Ezに対する係数を計算する
#pragma omp section
			{
				std::vector<cindex_t> normal_cindex(Nx * Ny * Nz, pec_id);
				std::vector<cindex2_t> pml_cindex;
				std::vector<index_t> pml_index;
				for (index_t ilz = 0; ilz < m_LocalSizeZ; ilz++){
					index_t iz = m_LocalOffsetZ + ilz;
					for (index_t iy = 1; iy < Ny; iy++){
						for (index_t ix = 1; ix < Nx; ix++){
							double dx = m_GridX.mwidth(ix);
							double dy = m_GridY.mwidth(iy);
							FFMaterial mat;
							bool pec = getMaterialEz(index3_t(ix, iy, iz), &mat);
							bool inside_pml =
								(iz < Lz) || ((Mz - 2 * Lz) <= iz) ||
								(iy <= Ly) || (((Ny - 2 * Ly - 1) < iy) && (0 < Ly)) ||
								(ix <= Lx) || (((Nx - 2 * Lx - 1) < ix) && (0 < Lx));
							if (inside_pml){
								double sigma_x_max = calcSigmaMax(mat.eps(), dx, Lx);
								double sigma_y_max = calcSigmaMax(mat.eps(), dy, Ly);
								double sigma_x = (0 < Lx) ? calcSigma(sigma_x_max, Lx, Nx - 2 * Lx - 1, ix, Lx) : 0.0;
								double sigma_y = (0 < Ly) ? calcSigma(sigma_y_max, Ly, Ny - 2 * Ly - 1, iy, Ly) : 0.0;
								pml_cindex.push_back(cindex2_t(
									registerCoef2(FFMaterial::calcDCoefPML(mat.eps_r(), sigma_x, Dt, dx)),
									registerCoef2(FFMaterial::calcDCoefPML(mat.eps_r(), sigma_y, Dt, dy))));
								pml_index.push_back(ix + Nx * (iy + Ny * ilz));
								normal_cindex[ix + Nx * (iy + Ny * ilz)] = pec ? pec_id : registerCoef2(mat.calcECoefPML(Dt));
							}
							else{
								normal_cindex[ix + Nx * (iy + Ny * ilz)] = pec ? pec_id : registerCoef3(mat.calcECoef(Dt, dx, dy));
							}
						}
					}
				}

#pragma omp critical
				m_Solver->storeCoefficientIndex(EMType::Ez, normal_cindex, pml_cindex, pml_index);
			}

			// Hxに対する係数を計算する
#pragma omp section
			{
				std::vector<cindex_t> normal_cindex(Nx * Ny * Nz, pec_id);
				std::vector<cindex2_t> pml_cindex;
				std::vector<index_t> pml_index;
				for (index_t ilz = 0; ilz < m_LocalSizeZ; ilz++){
					index_t iz = m_LocalOffsetZ + ilz;
					for (index_t iy = 0; iy < My; iy++){
						for (index_t ix = 1; ix < Nx; ix++){
							double dy = m_GridY.width(iy);
							double dz = m_GridZ.width(iz);
							FFMaterial mat;
							getMaterialHx(index3_t(ix, iy, iz), &mat);
							bool inside_pml =
								(iz < Lz) || ((Mz - 2 * Lz) <= iz) ||
								(iy < Ly) || ((My - 2 * Ly) <= iy) ||
								(ix <= Lx) || (((Nx - 2 * Lx - 1) < ix) && (0 < Lx));
							if (inside_pml){
								double sigma_m_y_max = calcSigmaMax(mat.mu(), dy, Ly);
								double sigma_m_z_max = calcSigmaMax(mat.mu(), dz, Lz);
								double sigma_m_y = (0 < Ly) ? calcSigma(sigma_m_y_max, Ly, Ny - 2 * Ly - 1, iy + 0.5, Ly) : 0.0;
								double sigma_m_z = (0 < Lz) ? calcSigma(sigma_m_z_max, Lz, Nz - 2 * Lz - 1, iz + 0.5, Lz) : 0.0;
								pml_cindex.push_back(cindex2_t(
									registerCoef2(FFMaterial::calcHCoefPML(mat.mu_r(), sigma_m_y, Dt, dy)),
									registerCoef2(FFMaterial::calcHCoefPML(mat.mu_r(), sigma_m_z, Dt, dz))));
								pml_index.push_back(ix + Nx * (iy + Ny * ilz));
							}
							normal_cindex[ix + Nx * (iy + Ny * ilz)] = registerCoef3(mat.calcHCoef(Dt, dy, dz));
						}
					}
				}

#pragma omp critical
				m_Solver->storeCoefficientIndex(EMType::Hx, normal_cindex, pml_cindex, pml_index);
			}

			// Hyに対する係数を計算する
#pragma omp section
			{
				std::vector<cindex_t> normal_cindex(Nx * Ny * Nz, pec_id);
				std::vector<cindex2_t> pml_cindex;
				std::vector<index_t> pml_index;
				for (index_t ilz = 0; ilz < m_LocalSizeZ; ilz++){
					index_t iz = m_LocalOffsetZ + ilz;
					for (index_t iy = 1; iy < Ny; iy++){
						for (index_t ix = 0; ix < Mx; ix++){
							double dz = m_GridZ.width(iz);
							double dx = m_GridX.width(ix);
							FFMaterial mat;
							getMaterialHy(index3_t(ix, iy, iz), &mat);
							bool inside_pml =
								(iz < Lz) || ((Mz - 2 * Lz) <= iz) ||
								(iy <= Ly) || (((Ny - 2 * Ly - 1) < iy) && (0 < Ly)) ||
								(ix < Lx) || ((Mx - 2 * Lx) <= ix);
							if (inside_pml){
								double sigma_m_z_max = calcSigmaMax(mat.mu(), dz, Lz);
								double sigma_m_x_max = calcSigmaMax(mat.mu(), dx, Lx);
								double sigma_m_z = (0 < Lz) ? calcSigma(sigma_m_z_max, Lz, Nz - 2 * Lz - 1, iz + 0.5, Lz) : 0.0;
								double sigma_m_x = (0 < Lx) ? calcSigma(sigma_m_x_max, Lx, Nx - 2 * Lx - 1, ix + 0.5, Lx) : 0.0;
								pml_cindex.push_back(cindex2_t(
									registerCoef2(FFMaterial::calcHCoefPML(mat.mu_r(), sigma_m_z, Dt, dz)),
									registerCoef2(FFMaterial::calcHCoefPML(mat.mu_r(), sigma_m_x, Dt, dx))));
								pml_index.push_back(ix + Nx * (iy + Ny * ilz));
							}
							normal_cindex[ix + Nx * (iy + Ny * ilz)] = registerCoef3(mat.calcHCoef(Dt, dz, dx));
						}
					}
				}
				
#pragma omp critical
				m_Solver->storeCoefficientIndex(EMType::Hy, normal_cindex, pml_cindex, pml_index);
			}

			// Hzに対する係数を計算する
#pragma omp section
			{
				std::vector<cindex_t> normal_cindex(Nx * Ny * Nz, pec_id);
				std::vector<cindex2_t> pml_cindex;
				std::vector<index_t> pml_index;
				for (index_t ilz = 1; ilz <= m_LocalSizeZ; ilz++){
					index_t iz = m_LocalOffsetZ + ilz;
					for (index_t iy = 0; iy < My; iy++){
						for (index_t ix = 0; ix < Mx; ix++){
							double dx = m_GridX.width(ix);
							double dy = m_GridY.width(iy);
							FFMaterial mat;
							getMaterialHz(index3_t(ix, iy, iz), &mat);
							bool inside_pml =
								(iz <= Lz) || (((Nz - 2 * Lz - 1) < iz) && (0 < Lz)) ||
								(iy < Ly) || ((My - 2 * Ly) <= iy) ||
								(ix < Lx) || ((Mx - 2 * Lx) <= ix);
							if (inside_pml){
								double sigma_m_x_max = calcSigmaMax(mat.mu(), dx, Lx);
								double sigma_m_y_max = calcSigmaMax(mat.mu(), dy, Ly);
								double sigma_m_x = (0 < Lx) ? calcSigma(sigma_m_x_max, Lx, Nx - 2 * Lx - 1, ix + 0.5, Lx) : 0.0;
								double sigma_m_y = (0 < Ly) ? calcSigma(sigma_m_y_max, Ly, Ny - 2 * Ly - 1, iy + 0.5, Ly) : 0.0;
								pml_cindex.push_back(cindex2_t(
									registerCoef2(FFMaterial::calcHCoefPML(mat.mu_r(), sigma_m_x, Dt, dx)),
									registerCoef2(FFMaterial::calcHCoefPML(mat.mu_r(), sigma_m_y, Dt, dy))));
								pml_index.push_back(ix + Nx * (iy + Ny * ilz));
							}
							normal_cindex[ix + Nx * (iy + Ny * ilz)] = registerCoef3(mat.calcHCoef(Dt, dx, dy));
						}
					}
				}
				
#pragma omp critical
				m_Solver->storeCoefficientIndex(EMType::Hz, normal_cindex, pml_cindex, pml_index);
			}
		}

		// 係数リストをコピーする
		m_Solver->storeCoefficientList(coef2_list, coef3_list);
		
		// 観測点・観測面・ポートの情報をコピーする
		m_Solver->storeMeasurementInfo(m_FreqList, m_NT, m_TDProbeList, m_FDProbeList);
		m_Solver->storePortList(m_PortList);
	}

	// 計算を1ステップ進める
	size_t FFSituation::stepSolver(int bottom_rank, int top_rank){
		if (m_Solver == nullptr){
			throw;
		}
		if (m_NT <= m_IT){
			throw;
		}

		// 給電・計測を行う
		m_Solver->feedAndMeasure(m_IT);
		m_IT++;

		if (m_IT < m_NT){
			// 磁界を計算する
			m_Solver->calcHField();

			// 端部の磁界を送受信
			if (isConnectedX()){
				m_Solver->exchangeEdgeH(Axis::X);
			}
			if (isConnectedY()){
				m_Solver->exchangeEdgeH(Axis::Y);
			}
			if (isConnectedZ()){
				if (m_LocalSizeZ == m_Size.z){
					m_Solver->exchangeEdgeH(Axis::Z);
				}
				else{
					/*std::vector<real> t_hx, t_hy, t_hz;
					m_Solver->getEdgeH(nullptr, nullptr, &t_hz);
					m_Solver->getEdgeH(&t_hx, &t_hy, nullptr);
					std::vector<real> r_hx(t_hx.size()), r_hy(t_hy.size()), r_hz(t_hz.size());
					MPI_Request req[6];
					MPI_Status status[6];
					int result_t_hx = MPI_Isend(t_hx.data(), (int)t_hx.size(), MPI_FLOAT, bottom_rank, (int)MPITag::Hx, MPI_COMM_WORLD, &req[0]);
					int result_t_hy = MPI_Isend(t_hy.data(), (int)t_hy.size(), MPI_FLOAT, bottom_rank, (int)MPITag::Hy, MPI_COMM_WORLD, &req[1]);
					int result_t_hz = MPI_Isend(t_hz.data(), (int)t_hz.size(), MPI_FLOAT, top_rank, (int)MPITag::Hz, MPI_COMM_WORLD, &req[2]);
					int result_r_hx = MPI_Irecv(r_hx.data(), (int)r_hx.size(), MPI_FLOAT, top_rank, (int)MPITag::Hx, MPI_COMM_WORLD, &req[3]);
					int result_r_hy = MPI_Irecv(r_hy.data(), (int)r_hy.size(), MPI_FLOAT, top_rank, (int)MPITag::Hy, MPI_COMM_WORLD, &req[4]);
					int result_r_hz = MPI_Irecv(r_hz.data(), (int)r_hz.size(), MPI_FLOAT, bottom_rank, (int)MPITag::Hz, MPI_COMM_WORLD, &req[5]);
					int result_wait = MPI_Waitall(sizeof(req) / sizeof(int), req, status);*/
				}
			}

			// 電界を計算する
			m_Solver->calcEField();

			// 端部の電界を送受信
			if (isConnectedX()){
				m_Solver->exchangeEdgeE(Axis::X);
			}
			if (isConnectedY()){
				m_Solver->exchangeEdgeE(Axis::Y);
			}
			if (isConnectedZ()){
				if (m_LocalSizeZ == m_Size.z){
					m_Solver->exchangeEdgeE(Axis::Z);
				}
				else{
					/*std::vector<real> t_ex, t_ey, t_ez;
					m_Solver->getEdgeE(nullptr, nullptr, &t_ez);
					m_Solver->getEdgeE(&t_ex, &t_ey, nullptr);
					std::vector<real> r_ex(t_ex.size()), r_ey(t_ey.size()), r_ez(t_ez.size());
					MPI_Request req[6];
					MPI_Status status[6];
					int result_t_ex = MPI_Isend(t_ex.data(), (int)t_ex.size(), MPI_FLOAT, top_rank, (int)MPITag::Ex, MPI_COMM_WORLD, &req[0]);
					int result_t_ey = MPI_Isend(t_ey.data(), (int)t_ey.size(), MPI_FLOAT, top_rank, (int)MPITag::Ey, MPI_COMM_WORLD, &req[1]);
					int result_t_ez = MPI_Isend(t_ez.data(), (int)t_ez.size(), MPI_FLOAT, bottom_rank, (int)MPITag::Ez, MPI_COMM_WORLD, &req[2]);
					int result_r_ex = MPI_Irecv(r_ex.data(), (int)r_ex.size(), MPI_FLOAT, bottom_rank, (int)MPITag::Ex, MPI_COMM_WORLD, &req[3]);
					int result_r_ey = MPI_Irecv(r_ey.data(), (int)r_ey.size(), MPI_FLOAT, bottom_rank, (int)MPITag::Ey, MPI_COMM_WORLD, &req[4]);
					int result_r_ez = MPI_Irecv(r_ez.data(), (int)r_ez.size(), MPI_FLOAT, top_rank, (int)MPITag::Ez, MPI_COMM_WORLD, &req[5]);
					int result_wait = MPI_Waitall(sizeof(req) / sizeof(int), req, status);*/
				}
			}
		}

		return m_IT;
	}


#pragma endregion




}


