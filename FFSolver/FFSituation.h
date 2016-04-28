#pragma once

#include "FFGrid.h"
#include "FFMaterial.h"
#include "FFPointObject.h"
//#include "Circuit/FFCircuit.h"
#include "Format/FFVolumeData.h"
#include "Format/FFBitVolumeData.h"
#include "Basic/FFIStream.h"



namespace FFFDTD{
	// 境界条件を格納する構造体
	struct BC_t{
		BoundaryCondition x, y, z;
		index3_t pmlL;
		double pmlM;
		double pmlR0;
	};

	

	// シミュレーション環境を作成するクラス
	class FFSituation{
		/*** 定数 ***/
	private:
		// PECの材質ID
		static const matid_t MATID_PEC = 0;



		/*** メンバー変数 ***/
	private:
		// グローバル領域のサイズ
		index3_t m_Size;

		// ローカル領域のオフセット
		index_t m_LocalOffsetZ;
		
		// ローカル領域のサイズ
		index_t m_LocalSizeZ;

		// Z方向の正端に接続される座標
		index_t m_ConnectionZ;

		// グリッド
		FFGrid m_GridX, m_GridY, m_GridZ;

		// 境界条件
		BC_t m_BC;

		// ボリュームデータ
		FFVolumeData m_Volume;

		// PECワイヤーのボリュームデータ
		FFBitVolumeData m_PECX, m_PECY, m_PECZ;

		// 材質リスト
		std::vector<FFMaterial*> m_MaterialList;

		// 観測点リスト(電界成分の位置)
		std::vector<FFPointObject> m_ProbePointList;

		// 観測面リスト(磁界成分の位置)
		std::vector<FFPointObject> m_ProbePlaneList;

		// ポートリスト
		



		/*** メソッド ***/
	public:
		// コンストラクタ
		FFSituation(void);

		// デストラクタ
		~FFSituation();

#pragma region 初期化関連のメソッド
	public:
		// グリッドを設定する
		void setGrids(const FFGrid &grid_x, const FFGrid &grid_y, const FFGrid &grid_z, const BC_t &bc);

		// 処理の分割を設定する
		void setDivision(index_t offset, index_t size);

		// ボリュームデータを作成する
		void createVolumeData(void);
#pragma endregion

#pragma region 材質関連のメソッド
		// 材質リストを初期化する
		void initializeMaterialList(size_t count);

		// 新しく材質データを登録する
		matid_t registerMaterial(FFMaterial *mat);

		// 指定した材質IDで材質データを登録する
		void registerMaterial(matid_t matid, FFMaterial *mat);

		// 材質リストのすべての材質の物性値が指定されているか調べる
		bool isMaterialListFilled(void);

		// 指定した材質IDの材質データを取得する
		const FFMaterial* getMaterialByID(matid_t matid);
#pragma endregion

#pragma region シミュレーション環境の情報を取得するメソッド
	public:
		// 最適なタイムステップを計算する
		double calcTimestep(void) const;

		// グローバル領域の大きさを取得する
		const index3_t getGlobalSize(void) const{
			return m_Size;
		}

		// ローカル領域の大きさを取得する
		index_t getLocalSize(void) const{
			return m_LocalSizeZ;
		}

		// ローカル領域のオフセットを取得する
		index_t getLocalOffset(void) const{
			return m_LocalOffsetZ;
		}

		// 境界条件を取得する
		BoundaryCondition getBC(AXIS_e axis) const;

		// PML吸収境界条件の層数を取得する
		const index3_t& getPmlL(void) const{
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
#pragma endregion
		
#pragma region シミュレーション環境を作成するメソッド
		// 指定した材質IDの直方体を配置する
		bool placeCuboid(matid_t matid, const index3_t &pos1, const index3_t &pos2);

		// PECワイヤーの直方体を配置する
		bool placePECCuboid(const index3_t &pos1, const index3_t &pos2);


		// PECデータをストリームから読み込む
		//void loadPECData(FFIStream &stream);

		// ボリュームデータをストリームから読み込む
		//void loadVolumeData(FFIStream &stream);

		// ボリュームデータとPECデータを現在のグリッド設定から作成する
		//void createVolumeAndPECDataFromGrid(void);



		// 観測点を配置する
		size_t placeProbePoint(const FFPointObject &object);

		// 観測面を配置する
		size_t placeProbePoint(const FFPointObject &object);

#pragma endregion

#pragma region 係数を計算するメソッド
	public:
		// Exに作用する物性値を取得する
		// PECワイヤーがあるときtrueを返す
		bool getMaterialEx(const index3_t &pos, FFMaterial *material) const;

		// Eyに作用する物性値を取得する
		// PECワイヤーがあるときtrueを返す
		bool getMaterialEy(const index3_t &pos, FFMaterial *material) const;

		// Ezに作用する物性値を取得する
		// PECワイヤーがあるときtrueを返す
		bool getMaterialEz(const index3_t &pos, FFMaterial *material) const;

		// Hxに作用する物性値を取得する
		void getMaterialHx(const index3_t &pos, FFMaterial *material) const;

		// Hyに作用する物性値を取得する
		void getMaterialHy(const index3_t &pos, FFMaterial *material) const;

		// Hzに作用する物性値を取得する
		void getMaterialHz(const index3_t &pos, FFMaterial *material) const;

		/*private:
		// グローバル座標から配列のインデックスを計算する
		size_t getIndex(index_t x, index_t y, index_t z) const{
			return (size_t)x + (size_t)m_Size.x * ((size_t)y + (size_t)m_Size.y * (size_t)(z - m_LocalOffsetZ));
		}

		// グローバル座標から配列のインデックスを計算する
		size_t getIndex(const index3_t &pos) const{
			return getIndex(pos.x, pos.y, pos.z);
		}*/
#pragma endregion












	};
}
