#pragma once

#include "FFMaterial.h"
#include <vector>



namespace MUFDTD{
	// 媒質の空間配置を保持するクラス
	class FFSpace{
		/*** 定数 ***/
	private:
		// 境界をチェックする
		static const bool CHECK_BOUNDARY = true;

	public:
		// 真空の物質ID
		static const matid_t MATID_VACUUM = 0;

		// PECの媒質ID
		static const matid_t MATID_PEC = 1;

		// 物質IDの最大値
		static const matid_t MATID_MAXIMUM = 65535;



		/*** メンバー変数 ***/
	private:
		// 空間の大きさ
		index_t m_Mx, m_My, m_Mz;
		
		// 空間の大きさ+1
		index_t m_Nx, m_Ny, m_Nz;
		
		// 媒質の空間配置
		std::vector<matid_t> m_Media;

		// PECワイヤーの空間配置
		std::vector<bool> m_PECX, m_PECY, m_PECZ;

		// 物性値情報
		std::vector<FFMaterial> m_Materials;



		/*** メソッド ***/
	public:
		// コンストラクタ
		FFSpace(index_t mx, index_t my, index_t mz);

		// 領域のX方向の大きさを取得する
		index_t getSizeX(void) const{
			return m_Mx;
		}

		// 領域のY方向の大きさを取得する
		index_t getSizeY(void) const{
			return m_My;
		}
		
		// 領域のZ方向の大きさを取得する
		index_t getSizeZ(void) const{
			return m_Mz;
		}

		// 物性値を登録し、物質IDを取得する
		matid_t registerMaterial(const FFMaterial &material);

		// 物質IDから物性値を取得する
		const FFMaterial* getMaterialByID(matid_t matid) const;

		// 登録された物質IDの数を取得する
		matid_t getNumberOfMeterials(void) const{
			return static_cast<matid_t>(m_Materials.size());
		}

		// X方向に媒質を配置する
		void placeMedium(matid_t matid, const index3_t &pos, index_t count, VoxelOPs opcode);

		// X方向に複数の媒質を配置する
		void placeMedia(const matid_t *matid, const index3_t &pos, index_t count, VoxelOPs opcode);

		// X方向にX方向のPECを配置する
		void placePECX(const index3_t &pos, index_t count);

		// X方向にY方向のPECを配置する
		void placePECY(const index3_t &pos, index_t count);

		// X方向にZ方向のPECを配置する
		void placePECZ(const index3_t &pos, index_t count);

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

	private:
		// 配列のインデックスを計算する
		size_t getIndex(index_t x, index_t y, index_t z) const{
			return x + m_Mx * (y + m_My * z);
		}

		// 配列のインデックスを計算する
		size_t getIndex(const index3_t &pos) const{
			return pos.x + m_Mx * (pos.y + m_My * pos.z);
		}

		// 指定された座標の媒質の物性値を取得する
		const FFMaterial& getMaterial(index_t x, index_t y, index_t z) const{
			return m_Materials[m_Media[getIndex(x, y, z)]];
		}
	};
}
