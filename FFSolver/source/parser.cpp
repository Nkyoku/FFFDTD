#include "parser.h"
#include "FFConst.h"
#include "Basic/FFException.h"
#include "Circuit/FFVoltageSourceComponent.h"



namespace Parser{
	// msgpackのエラーを取得し、内部のエラーであれば例外を発生する
	static mpack_error_t msgpackError(mpack_node_t &node){
		mpack_error_t error = mpack_node_error(node);
		switch (error){
		case mpack_ok:
			break;
		case mpack_error_io:
			throw "Msgpack I/O error";
		case mpack_error_invalid:
			throw "Msgpack data invalid";
		case mpack_error_type:
			break;
		case mpack_error_too_big:
			throw "Msgpack too big";
		case mpack_error_memory:
			throw "Msgpack failed to memory allocation";
		case mpack_error_bug:
			throw "Msgpack API error";
		case mpack_error_data:
			break;
		}
		return error;
	}

	// 配列をパースする
	template<typename T, T func(mpack_node_t node)>
	static std::vector<T> getArray(mpack_node_t &node){
		if (mpack_node_type(node) == mpack_type_array){
			size_t count = mpack_node_array_length(node);
			std::vector<double> result(count);
			for (size_t i = 0; i < count; i++){
				result[i] = func(mpack_node_array_at(node, i));
			}
			return result;
		}
		else{
			mpack_node_flag_error(node, mpack_error_type);
			return std::vector<T>();
		}
	}

	// 配列を3次元ベクトルとしてパースする
	template<typename T, T func(mpack_node_t node)>
	static tvec3<T> getVec3(mpack_node_t &node){
		if (mpack_node_array_length(node) == 3){
			tvec3<T> result;
			result.x = func(mpack_node_array_at(node, 0));
			result.y = func(mpack_node_array_at(node, 1));
			result.z = func(mpack_node_array_at(node, 2));
			return result;
		}
		else{
			mpack_node_flag_error(node, mpack_error_type);
			return tvec3<T>();
		}
	}

	// ノードから文字列を取得する
	static std::string getString(mpack_node_t &node){
		return std::string(mpack_node_str(node), mpack_node_strlen(node));
	}

	// ノードが指定された文字列と一致するか調べる
	static bool compareToString(mpack_node_t &node, const char *text){
		if (mpack_node_type(node) != mpack_type_str){
			return false;
		}
		const char *str = mpack_node_str(node);
		size_t len = mpack_node_strlen(node);
		if (str == nullptr){
			return false;
		}
		if (strlen(text) != len){
			return false;
		}
		return (strncmp(str, text, len) == 0);
	}

	// 方向を取得する
	static DIR_e getDir(mpack_node_t &node){
		const char *text = mpack_node_str(node);
		size_t len = mpack_node_strlen(node);
		if (len == 2){
			char sign = text[0];
			char axis = text[1];
			if ((sign == '+') && (axis == 'X')){
				return X_PLUS;
			}
			else if ((sign == '+') && (axis == 'Y')){
				return Y_PLUS;
			}
			else if ((sign == '+') && (axis == 'Z')){
				return Z_PLUS;
			}
			else if ((sign == '-') && (axis == 'X')){
				return X_MINUS;
			}
			else if ((sign == '-') && (axis == 'Y')){
				return Y_MINUS;
			}
			else if ((sign == '-') && (axis == 'Z')){
				return Z_MINUS;
			}
		}
		mpack_node_flag_error(node, mpack_error_data);
		return X_PLUS;
	}

	// msgpackノードからグリッドと境界条件をパースする
	index3_t parseGridAndBC(mpack_node_t root_node, std::vector<FFSituation> &situation_list){
		try{
			FFGrid grid_x, grid_y, grid_z;
			BC_t bc;

			// 文字列から境界条件を判別する
			auto string_to_bc = [&](mpack_node_t &node) -> BoundaryCondition{
				char buf[32];
				size_t size = mpack_node_copy_utf8(node, buf, sizeof(buf) - 1);
				buf[size] = '\0';
				if (strcmp(buf, "PEC") == 0){
					return BoundaryCondition::PEC;
				}
				else if (strcmp(buf, "PML") == 0){
					return BoundaryCondition::PML;
				}
				else if (strcmp(buf, "Periodic") == 0){
					return BoundaryCondition::Periodic;
				}
				else{
					mpack_node_flag_error(node, mpack_error_data);
					return BoundaryCondition::PEC;
				}
			};

			// グリッドをパースする
			mpack_node_t grid_node = mpack_node_map_cstr(root_node, "Grid");
			grid_x = getArray<double, mpack_node_double>(mpack_node_array_at(grid_node, 0));
			grid_y = getArray<double, mpack_node_double>(mpack_node_array_at(grid_node, 1));
			grid_z = getArray<double, mpack_node_double>(mpack_node_array_at(grid_node, 2));
			if (msgpackError(grid_node) != mpack_ok){
				throw "Grid information";
			}

			// 境界条件をパースする
			mpack_node_t bc_node = mpack_node_map_cstr(root_node, "BoundaryCondition");
			bc.x = string_to_bc(mpack_node_array_at(bc_node, 0));
			bc.y = string_to_bc(mpack_node_array_at(bc_node, 1));
			bc.z = string_to_bc(mpack_node_array_at(bc_node, 2));
			if (msgpackError(bc_node) != mpack_ok){
				throw "Boundary conditions";
			}

			// PMLパラメータをパースする
			mpack_node_t pml_node = mpack_node_map_cstr_optional(root_node, "PML");
			if (mpack_node_type(pml_node) != mpack_type_nil){
				mpack_node_t pml_l_node = mpack_node_map_cstr(pml_node, "Layers");
				mpack_node_t pml_m_node = mpack_node_map_cstr(pml_node, "Order");
				mpack_node_t pml_r0_node = mpack_node_map_cstr(pml_node, "R0");
				bc.pmlL = getVec3<index_t, mpack_node_u32>(pml_l_node);
				bc.pmlM = mpack_node_double(pml_m_node);
				bc.pmlR0 = mpack_node_double(pml_r0_node);
				if (msgpackError(pml_node) != mpack_ok){
					throw "PML parameters";
				}
			}

			if (msgpackError(bc_node) != mpack_ok){
				throw "Unknown";
			}

			// FFSituationに設定する
			for (auto &situation : situation_list){
				situation.setGrids(grid_x, grid_y, grid_z, bc);
			}

			return index3_t(grid_x.count(), grid_y.count(), grid_z.count());
		}
		catch (const char *msg){
			throw FFException("Parse error '%s'", msg);
		}
	}

	// msgpackノードから媒質の物性情報をパースする
	void parseMaterials(mpack_node_t root_node, std::vector<FFSituation> &situation_list){
		try{
			size_t count = mpack_node_array_length(root_node);	// この材質リストにMATID=0は含まれない
			if (MAX_MATID < (count + 1)){
				throw "Material count is too much";
			}

			for (auto &situation : situation_list){
				situation.initializeMaterialList(count + 1);
			}

			for (size_t i = 0; i < count; i++){
				mpack_node_t node = mpack_node_array_at(root_node, i);

				mpack_node_t eps_node = mpack_node_map_cstr_optional(node, "Epsilon");
				mpack_node_t sigma_node = mpack_node_map_cstr_optional(node, "Sigma");
				mpack_node_t mu_node = mpack_node_map_cstr_optional(node, "Mu");

				double eps = (mpack_node_type(eps_node) != mpack_type_nil) ? mpack_node_double(eps_node) : 1.0;
				double sigma = (mpack_node_type(sigma_node) != mpack_type_nil) ? mpack_node_double(sigma_node) : 0.0;
				double mu = (mpack_node_type(mu_node) != mpack_type_nil) ? mpack_node_double(mu_node) : 1.0;

				if (msgpackError(root_node) != mpack_ok){
					throw "Material information";
				}

				for (auto &situation : situation_list){
					situation.registerMaterial((matid_t)(i + 1), new FFMaterial(eps, sigma, mu));
				}
			}
		}
		catch (const char *msg){
			throw FFException("Parse error '%s'", msg);
		}
	}

	// msgpackノードから物体情報をパースする
	void parseObjects(mpack_node_t root_node, std::vector<FFSituation> &situation_list){
		try{
			size_t count = mpack_node_array_length(root_node);
			for (size_t i = 0; i < count; i++){
				mpack_node_t node = mpack_node_array_at(root_node, i);

				mpack_node_t mat_node = mpack_node_map_cstr(node, "Material");
				std::string type = getString(mpack_node_map_cstr(node, "Type"));

				bool pec = compareToString(mat_node, "PEC");
				matid_t matid = (pec == false) ? mpack_node_u16(mat_node) : 0;

				if (msgpackError(root_node) != mpack_ok){
					throw "Object information";
				}

				if (type.compare("Cuboid") == 0){
					index3_t start = getVec3<index_t, mpack_node_u32>(mpack_node_map_cstr(node, "Start"));
					index3_t end = getVec3<index_t, mpack_node_u32>(mpack_node_map_cstr(node, "End"));
					if (pec == true){
						for (auto &situation : situation_list){
							situation.placePECCuboid(start, end);
						}
					}
					else{
						for (auto &situation : situation_list){
							situation.placeCuboid(matid, start, end);
						}
					}
				}
				else{
					throw "Unknown object type";
				}

				if (msgpackError(root_node) != mpack_ok){
					throw "Object information";
				}
			}
		}
		catch (const char *msg){
			throw FFException("Parse error '%s'", msg);
		}
	}

	// msgpackノードからポート情報をパースする
	void parsePorts(mpack_node_t root_node, std::vector<FFSituation> &situation_list){
		try{
			size_t count = mpack_node_array_length(root_node);
			for (size_t i = 0; i < count; i++){
				mpack_node_t node = mpack_node_array_at(root_node, i);

				index3_t pos = getVec3<index_t, mpack_node_u32>(mpack_node_map_cstr(node, "Position"));
				DIR_e dir = getDir(mpack_node_map_cstr(node, "Direction"));
				std::string type = getString(mpack_node_map_cstr(node, "Type"));

				if (msgpackError(root_node) != mpack_ok){
					throw "Port information";
				}

				if (type.compare("VoltageSource") == 0){
					// 電圧源
					mpack_node_t esr_node = mpack_node_map_cstr_optional(node, "ESR");
					double esr = (mpack_node_type(esr_node) == mpack_type_nil) ? 0.0 : mpack_node_double(esr_node);
					std::string waveform = getString(mpack_node_map_cstr(node, "Waveform"));

					for (auto &situation : situation_list){
						situation.placePort(pos, dir, new FFVoltageSourceComponent(new FFWaveform(waveform), esr));
					}
				}
				else{
					throw "Unknown port type";
				}

				if (msgpackError(root_node) != mpack_ok){
					throw "Port information";
				}
			}
		}
		catch (const char *msg){
			throw FFException("Parse error '%s'", msg);
		}
	}

	// msgpackノードからソルバー情報をパースする
	size_t parseSolvers(mpack_node_t root_node, std::vector<FFSituation> &situation_list, std::vector<FFSolver*> &solver_list, double optimum_timestep){
		try{
			// タイムステップ
			mpack_node_t timestep_node = mpack_node_map_cstr(root_node, "Timestep");
			double timestep = (mpack_node_type(timestep_node) == mpack_type_str) ? optimum_timestep : mpack_node_double(timestep_node);
			
			// 解析周波数のリスト
			std::vector<double> freq_list = getArray<double, mpack_node_double>(mpack_node_map_cstr(root_node, "Frequency"));

			// 計算ステップ数
			size_t iteration = mpack_node_u32(mpack_node_map_cstr(root_node, "Iteration"));

			if (msgpackError(root_node) != mpack_ok){
				throw "Solver information";
			}

			// ソルバーを構成する
#pragma omp parallel for
			for (int i = 0; i < (int)situation_list.size(); i++){
				situation_list[i].configureSolver(solver_list[i], timestep, iteration, freq_list);
			}

			return iteration;
		}
		catch (const char *msg){
			throw FFException("Parse error '%s'", msg);
		}
	}


}
