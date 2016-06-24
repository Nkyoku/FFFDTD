#include "solver_setting.h"
#include "FFSolverCPU.h"
#include "CL/cl.h"
#include "inih/ini.h"
#include <stdlib.h>
#include <algorithm>
#include <regex>

#if defined(_WIN32)
#define NOMINMAX
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#elif defined(__GNUC__)
#include <unistd.h>
#include <strings.h>
#endif



namespace SolverSetting{
	using namespace FFFDTD;

	// CPUソルバーのスレッド数
	static struct{
		int num_of_threads;
		uint64_t speed;
	} g_CPUInfo = {-1, 1};

	// OpenCLデバイス情報
	struct GPUInfo_t{
		bool used;
		std::string name;
		uint64_t speed;
		cl_device_id device_id;
	};
	static std::vector<GPUInfo_t> g_GPUInfo;



	// ホスト名を取得する
	void getHostname(char *buf, size_t length){
		if (gethostname(buf, (int)length) == 0){
			if (0 < length){
				buf[length - 1] = '\0';
			}
		}
		else{
			if (0 < length){
				buf[0] = '\0';
			}
		}
	}

	// OpenCLデバイスIDを列挙する
	static void enumerateOpenCL(void){
		cl_int ret;

		// プラットフォーム数を取得する
		cl_uint num_of_platforms;
		ret = clGetPlatformIDs(0, nullptr, &num_of_platforms);
		if ((ret != CL_SUCCESS) || (num_of_platforms == 0)){
			// エラーまたはOpenCL対応デバイスが見つからない
			return;
		}

		// プラットフォームを列挙する
		std::vector<cl_platform_id> platform_list(num_of_platforms);
		ret = clGetPlatformIDs(num_of_platforms, platform_list.data(), nullptr);
		if (ret != CL_SUCCESS){
			// エラー
			return;
		}
		for (cl_uint i = 0; i < num_of_platforms; i++){
			// デバイス数を取得する
			cl_uint num_of_devices;
			ret = clGetDeviceIDs(platform_list[i], CL_DEVICE_TYPE_GPU, 0, nullptr, &num_of_devices);
			if ((ret != CL_SUCCESS) || (num_of_devices == 0)){
				// エラーまたはOpenCL対応デバイスが見つからない
				continue;
			}

			// デバイスを列挙する
			std::vector<cl_device_id> device_list(num_of_devices);
			ret = clGetDeviceIDs(platform_list[i], CL_DEVICE_TYPE_GPU, num_of_devices, device_list.data(), nullptr);
			if (ret != CL_SUCCESS){
				// エラー
				continue;
			}
			for (cl_uint j = 0; j < num_of_devices; j++){
				// デバイス名を取得する
				size_t name_length;
				clGetDeviceInfo(device_list[j], CL_DEVICE_NAME, 0, nullptr, &name_length);
				std::vector<char> name(name_length);
				clGetDeviceInfo(device_list[j], CL_DEVICE_NAME, name_length, name.data(), nullptr);

				// デバイスIDリストに格納する
				GPUInfo_t info;
				info.used = false;
				info.name = name.data();
				info.speed = 0;
				info.device_id = device_list[j];
				g_GPUInfo.push_back(info);
			}
		}
	}

	// 文字列を比較する
	static bool compare(const char *s1, const char *s2){
#if defined(_WIN32)
		return (_stricmp(s1, s2) == 0);
#elif defined(__GNUC__)
		return (strcasecmp(s1, s2) == 0);
#endif
	}

	// INIパーサーのコールバック関数
	static int iniCallback(void *user, const char *section, const char *name, const char *value){
		const char *hostname = (char*)user;
		if (compare(section, "default") || compare(section, hostname)){
			// 形式
			// <ソルバー種類名> = <ソルバーオプション>, <計算速度>
			
			std::vector<char> option_vec(strlen(value) + 1);
			uint64_t speed;
			int count = sscanf(value, "%[^,],%llu", option_vec.data(), &speed);
			if (1 <= count){
				std::string option(option_vec.data());
				if (count == 1){
					speed = 0;
				}

				if (compare(name, "CPU")){
					// CPUソルバー

					// スレッド数の設定値を取得する
					int threads = compare(option.c_str(), "auto") ? 0 : atoi(option.c_str());

					// 情報を格納する
					g_CPUInfo.num_of_threads = std::max(threads, g_CPUInfo.num_of_threads);
					g_CPUInfo.speed = std::max(speed, g_CPUInfo.speed);

					return 1;
				}
				else if (compare(name, "GPU")){
					// GPUソルバー

					// 名前が一致する未使用のデバイスに使用中フラグを立てる
					std::regex expr(option);
					for (auto &info : g_GPUInfo){
						if ((info.used == false) && (std::regex_match(info.name, expr) == true)){
							info.used = true;
							info.speed = std::max(speed, info.speed);
							return 1;
						}
					}

					printf("Warning : GPU solver '%s' was not found\n", option.c_str());
					return 0;
				}
			}
			printf("Warning : Solver definition '%s=%s' is invalid\n", name, value);
			return 0;
		}
		return 1;
	}

	// ソルバー設定ファイルを読み込みソルバーを作成する
	void createSolvers(const char *path, const char *hostname, std::vector<FFFDTD::FFSolver*> *solver_list, std::vector<uint64_t> *speed_list){
		// OpenCLデバイスを列挙する
		enumerateOpenCL();
		
		if (0 <= ini_parse(path, iniCallback, const_cast<char*>(hostname))){
			// INIファイルを読み込めた

			// CPUソルバーを作成する
			if (0 <= g_CPUInfo.num_of_threads){
				solver_list->push_back(FFSolverCPU::createSolver(g_CPUInfo.num_of_threads));
				speed_list->push_back(g_CPUInfo.speed);
			}

			// GPUソルバーを作成する
			for (auto &info : g_GPUInfo){
				if (info.used == true){
					//solver_list->push_back(FFSolverGPU::createSolver(info.device_id));
					//speed_list->push_back(info.speed);
				}
			}
		}
		else{
			// INIファイルを開くことに失敗した
			puts("Warning : Cannot open solver settings");

			// CPUソルバーを作成する
			solver_list->push_back(FFSolverCPU::createSolver());
			speed_list->push_back(g_CPUInfo.speed);
		}
	}
}
