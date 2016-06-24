#pragma once

#include "FFSituation.h"
#include "mpack/mpack.h" 



namespace Parser{
	using namespace FFFDTD;

	// msgpackノードからグリッドと境界条件をパースする
	index3_t parseGridAndBC(mpack_node_t root_node, std::vector<FFSituation> &situation_list);

	// msgpackノードから媒質の物性情報をパースする
	void parseMaterials(mpack_node_t root_node, std::vector<FFSituation> &situation_list);

	// msgpackノードから物体情報をパースする
	void parseObjects(mpack_node_t root_node, std::vector<FFSituation> &situation_list);

	// msgpackノードからポート情報をパースする
	void parsePorts(mpack_node_t root_node, std::vector<FFSituation> &situation_list);

	// msgpackノードからソルバー情報をパースし、ソルバーを構成する
	size_t parseSolvers(mpack_node_t root_node, std::vector<FFSituation> &situation_list, std::vector<FFSolver*> &solver_list, double optimum_timestep);

}
