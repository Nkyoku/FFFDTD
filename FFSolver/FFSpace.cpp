#include "FFSpace.h"



namespace MUFDTD{
	// コンストラクタ
	FFSpace::FFSpace(index_t mx, index_t my, index_t mz)
		: m_Mx(mx), m_My(my), m_Mz(mz)
		, m_Nx(mx + 1), m_Ny(my + 1), m_Nz(mz + 1)
		, m_Media(mx * my * (mz + 1), MATID_VACUUM)
		, m_PECX(mx * my * (mz + 1), false), m_PECY(mx * my * (mz + 1), false), m_PECZ(mx * my * mz, false)
	{
		// 真空とPECの物性値を登録する
		registerMaterial(FFMaterial::vacuum());	// 真空
		registerMaterial(FFMaterial::vacuum());	// PEC
	}

	// 物性値を登録し、物質IDを取得する
	matid_t FFSpace::registerMaterial(const FFMaterial &material){
		if (m_Materials.size() == ((size_t)MATID_MAXIMUM + 1)) throw;
		m_Materials.push_back(material);
		return static_cast<matid_t>(m_Materials.size() - 1);
	}

	// 物質IDから物性値を取得する
	const FFMaterial* FFSpace::getMaterialByID(matid_t matid) const{
		if ((size_t)matid < m_Materials.size()){
			return &m_Materials[matid];
		}
		else{
			return nullptr;
		}
	}

	// X方向に媒質を配置する
	void FFSpace::placeMedium(matid_t matid, const index3_t &pos, index_t count, VoxelOPs opcode){
		if (CHECK_BOUNDARY && ((m_Mx < (pos.x + count)) || (m_My <= pos.y) || (m_Nz <= pos.z))) throw;
		if (count == 0) return;
		matid_t *dst = &m_Media[getIndex(pos)];
		switch (opcode){
		case VoxelOPs::Overwrite:
			while (0 < count--){
				*dst++ = matid;
			}
			break;
		case VoxelOPs::Add:
			if (matid != MATID_VACUUM){
				while (0 < count--){
					*dst++ = matid;
				}
			}
			break;
		case VoxelOPs::Minus:
			if (matid != MATID_VACUUM){
				while (0 < count--){
					*dst++ = MATID_VACUUM;
				}
			}
			break;
		case VoxelOPs::Intersection:
			if (matid != MATID_VACUUM){
				while (0 < count--){
					*dst = (*dst == MATID_VACUUM) ? matid : MATID_VACUUM;
					dst++;
				}
			}
			break;
		}
	}

	// X方向に複数の媒質を配置する
	void FFSpace::placeMedia(const matid_t *matid, const index3_t &pos, index_t count, VoxelOPs opcode){
		if (CHECK_BOUNDARY && ((m_Mx < (pos.x + count)) || (m_My <= pos.y) || (m_Nz <= pos.z))) throw;
		if (count == 0) return;
		matid_t *dst = &m_Media[getIndex(pos)];
		switch (opcode){
		case VoxelOPs::Overwrite:
			while(0 < count--){
				*dst++ = *matid++;
			}
			break;
		case VoxelOPs::Add:
			while (0 < count--){
				if (*matid != MATID_VACUUM) *dst = *matid;
				dst++;
				matid++;
			}
			break;
		case VoxelOPs::Minus:
			while (0 < count--){
				if (*matid != MATID_VACUUM) *dst = MATID_VACUUM;
				dst++;
				matid++;
			}
			break;
		case VoxelOPs::Intersection:
			while (0 < count--){
				if (*matid != MATID_VACUUM){
					*dst = (*dst == MATID_VACUUM) ? *matid : MATID_VACUUM;
				}
				dst++;
				matid++;
			}
			break;
		}
	}

	// X方向にX方向のPECを配置する
	void FFSpace::placePECX(const index3_t &pos, index_t count){
		if (CHECK_BOUNDARY && ((m_Mx < (pos.x + count)) || (m_Ny <= pos.y) || (m_Nz <= pos.z))) throw;
		if (count == 0) return;
		index_t offset = getIndex(pos.x, (pos.y != m_My) ? pos.y : 0, pos.z);
		for (index_t ix = 0; ix < count; ix++){
			m_PECX[offset + ix] = true;
		}
	}

	// X方向にY方向のPECを配置する
	void FFSpace::placePECY(const index3_t &pos, index_t count){
		if (CHECK_BOUNDARY && ((m_Nx < (pos.x + count)) || (m_My <= pos.y) || (m_Nz <= pos.z))) throw;
		if (count == 0) return;
		index_t offset = getIndex(pos);
		if (m_Nx == (pos.x + count)){
			m_PECY[offset - pos.x] = true;
			count--;
		}
		for (index_t ix = 0; ix < count; ix++){
			m_PECY[offset + ix] = true;
		}
	}

	// X方向にZ方向のPECを配置する
	void FFSpace::placePECZ(const index3_t &pos, index_t count){
		if (CHECK_BOUNDARY && ((m_Nx < (pos.x + count)) || (m_Ny <= pos.y) || (m_Mz <= pos.z))) throw;
		if (count == 0) return;
		index_t offset = getIndex(pos.x, (pos.y != m_My) ? pos.y : 0, pos.z);
		if (m_Nx == (pos.x + count)){
			m_PECZ[offset - pos.x] = true;
			count--;
		}
		for (index_t ix = 0; ix < count; ix++){
			m_PECZ[offset + ix] = true;
		}
	}

	// Exに作用する物性値を取得する
	bool FFSpace::getMaterialEx(const index3_t &pos, FFMaterial *material) const{
		if (CHECK_BOUNDARY && ((pos.y == 0) || (pos.z == 0) || (m_Mx <= pos.x) || (m_Ny <= pos.y) || (m_Nz <= pos.z))) throw;
		index_t y0 = pos.y - 1;
		index_t y1 = pos.y % m_My;
		const FFMaterial &mat1 = getMaterial(pos.x, y1, pos.z);
		const FFMaterial &mat2 = getMaterial(pos.x, y0, pos.z);
		const FFMaterial &mat3 = getMaterial(pos.x, y1, pos.z - 1);
		const FFMaterial &mat4 = getMaterial(pos.x, y0, pos.z - 1);
		*material = FFMaterial(
			(mat1.eps_r() + mat2.eps_r() + mat3.eps_r() + mat4.eps_r()) * 0.25,
			(mat1.sigma() + mat2.sigma() + mat3.sigma() + mat4.sigma()) * 0.25,
			(mat1.mu_r() + mat2.mu_r() + mat3.mu_r() + mat4.mu_r()) * 0.25);
		return m_PECX[pos.x + m_Mx * (y1 + m_My * pos.z)];
	}

	// Eyに作用する物性値を取得する
	bool FFSpace::getMaterialEy(const index3_t &pos, FFMaterial *material) const{
		if (CHECK_BOUNDARY && ((pos.x == 0) || (pos.z == 0) || (m_Nx <= pos.x) || (m_My <= pos.y) || (m_Nz <= pos.z))) throw;
		index_t x0 = pos.x - 1;
		index_t x1 = pos.x % m_Mx;
		const FFMaterial &mat1 =getMaterial(x1, pos.y, pos.z);
		const FFMaterial &mat2 =getMaterial(x0, pos.y, pos.z);
		const FFMaterial &mat3 =getMaterial(x1, pos.y, pos.z - 1);
		const FFMaterial &mat4 =getMaterial(x0, pos.y, pos.z - 1);
		*material = FFMaterial(
			(mat1.eps_r() + mat2.eps_r() + mat3.eps_r() + mat4.eps_r()) * 0.25,
			(mat1.sigma() + mat2.sigma() + mat3.sigma() + mat4.sigma()) * 0.25,
			(mat1.mu_r() + mat2.mu_r() + mat3.mu_r() + mat4.mu_r()) * 0.25);
		return m_PECY[x1 + m_Mx * (pos.y + m_My * pos.z)];
	}

	// Ezに作用する物性値を取得する
	bool FFSpace::getMaterialEz(const index3_t &pos, FFMaterial *material) const{
		if (CHECK_BOUNDARY && ((pos.x == 0) || (pos.y == 0) || (m_Nx <= pos.x) || (m_Ny <= pos.y) || (m_Mz <= pos.z))) throw;
		index_t x0 = pos.x - 1;
		index_t x1 = pos.x % m_Mx;
		index_t y0 = pos.y - 1;
		index_t y1 = pos.y % m_My;
		const FFMaterial &mat1 = getMaterial(x1, y1, pos.z);
		const FFMaterial &mat2 = getMaterial(x0, y1, pos.z);
		const FFMaterial &mat3 = getMaterial(x1, y0, pos.z);
		const FFMaterial &mat4 = getMaterial(x0, y0, pos.z);
		*material = FFMaterial(
			(mat1.eps_r() + mat2.eps_r() + mat3.eps_r() + mat4.eps_r()) * 0.25,
			(mat1.sigma() + mat2.sigma() + mat3.sigma() + mat4.sigma()) * 0.25,
			(mat1.mu_r() + mat2.mu_r() + mat3.mu_r() + mat4.mu_r()) * 0.25);
		return m_PECZ[x1 + m_Mx * (y1 + m_Ny * pos.z)];
	}

	// Hxに作用する物性値を取得する
	void FFSpace::getMaterialHx(const index3_t &pos, FFMaterial *material) const{
		if (CHECK_BOUNDARY && ((pos.x == 0) || (m_Nx <= pos.x) || (m_My <= pos.y) || (m_Mz <= pos.z))) throw;
		index_t x0 = pos.x - 1;
		index_t x1 = pos.x % m_Mx;
		const FFMaterial &mat1 = getMaterial(x1, pos.y, pos.z);
		const FFMaterial &mat2 = getMaterial(x0, pos.y, pos.z);
		*material = FFMaterial(
			(mat1.eps_r() + mat2.eps_r()) * 0.5,
			(mat1.sigma() + mat2.sigma()) * 0.5,
			(mat1.mu_r() + mat2.mu_r()) * 0.5);
	}

	// Hyに作用する物性値を取得する
	void FFSpace::getMaterialHy(const index3_t &pos, FFMaterial *material) const{
		if (CHECK_BOUNDARY && ((pos.y == 0) || (m_Mx <= pos.x) || (m_Ny <= pos.y) || (m_Mz <= pos.z))) throw;
		index_t y0 = pos.y - 1;
		index_t y1 = pos.y % m_My;
		const FFMaterial &mat1 = getMaterial(pos.x, y1, pos.z);
		const FFMaterial &mat2 = getMaterial(pos.x, y0, pos.z);
		*material = FFMaterial(
			(mat1.eps_r() + mat2.eps_r()) * 0.5,
			(mat1.sigma() + mat2.sigma()) * 0.5,
			(mat1.mu_r() + mat2.mu_r()) * 0.5);
	}

	// Hzに作用する物性値を取得する
	void FFSpace::getMaterialHz(const index3_t &pos, FFMaterial *material) const{
		if (CHECK_BOUNDARY && ((pos.z == 0) || (m_Mx <= pos.x) || (m_My <= pos.y) || (m_Nz <= pos.z))) throw;
		const FFMaterial &mat1 = getMaterial(pos.x, pos.y, pos.z);
		const FFMaterial &mat2 = getMaterial(pos.x, pos.y, pos.z - 1);
		*material = FFMaterial(
			(mat1.eps_r() + mat2.eps_r()) * 0.5,
			(mat1.sigma() + mat2.sigma()) * 0.5,
			(mat1.mu_r() + mat2.mu_r()) * 0.5);
	}
}


