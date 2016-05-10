// シミュレータ本体

#include "FFSituation.h"
#include "Circuit/FFVoltageSourceComponent.h"

#include <stdio.h>
#include <chrono>



using namespace FFFDTD;
using namespace glm;



template<typename T>
static std::vector<T> constspace(T value, int n){
	std::vector<T> output;
	for (int i = 0; i < n; i++){
		output.push_back(value);
	}
	return output;
}

template<typename T>
static std::vector<double> linspace(T start, T end, int n){
	std::vector<T> output;
	if (0 < n){
		output.push_back(start);
		for (int i = 1; i < n; i++){
			output.push_back(start + (end - start) * i / (n - 1));
		}
	}
	return output;
}



// メイン
int main(int argc, char *argv[]){
	// FFSituationの生成
	FFGrid grid_x = constspace(1.0, 50);
	FFGrid grid_y = constspace(1.0, 50);
	FFGrid grid_z = constspace(1.0, 151);
	BC_t bc = {
		BoundaryCondition::PML,
		BoundaryCondition::PML,
		BoundaryCondition::PML,
		index3_t(5, 5, 5),
		2.0,
		1e-5
	};
	FFSituation situation;
	situation.setGrids(grid_x, grid_y, grid_z, bc);
	situation.setDivision(0, 14);
	situation.createVolumeData();

	index3_t global_size = situation.getGlobalSize();
	printf("Situation1 :\n");
	printf("\tGlobalSize  : %dx%dx%d\n", global_size.x, global_size.y, global_size.z);
	printf("\tLocalSize   : %d\n", situation.getLocalSize());
	printf("\tLocalOffset : %d\n", situation.getLocalOffset());

	//situation.placeCuboid(10, index3_t(0, 0, 0), index3_t(10, 12, 14));
	situation.placePECCuboid(index3_t(25, 25, 25), index3_t(25, 25, 101));

	const char diffgauss[] =
		"var tw := 1.27 / 1.5e+9; \n"
		"var tmp := 4 * (t - tw) / tw; \n"
		"return [sqrt(2 * 2.718281828459045) * tmp * exp(-tmp * tmp)];";
	situation.placePort(FFPointObject(index3_t(25, 25, 75), Z_PLUS), new FFVoltageSourceComponent(new FFWaveform(diffgauss), 50.0));



	printf("\nPress any key.\n");
	getchar();
	
	return 0;
}
