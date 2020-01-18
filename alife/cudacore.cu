#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include <cuda.h>

inline int _ConvertSMVer2Cores(int major, int minor) {
	// Defines for GPU Architecture types (using the SM version to determine
	// the # of cores per SM
	typedef struct {
		int SM;  // 0xMm (hexidecimal notation), M = SM Major version,
		// and m = SM minor version
		int Cores;
	} sSMtoCores;

	sSMtoCores nGpuArchCoresPerSM[] = {
			{0x30, 192},
			{0x32, 192},
			{0x35, 192},
			{0x37, 192},
			{0x50, 128},
			{0x52, 128},
			{0x53, 128},
			{0x60, 64},
			{0x61, 128},
			{0x62, 128},
			{0x70, 64},
			{0x72, 64},
			{0x75, 64},
			{-1, -1}};

	int index = 0;

	while (nGpuArchCoresPerSM[index].SM != -1) {
		if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor)) {
			return nGpuArchCoresPerSM[index].Cores;
		}

		index++;
	}

	// If we don't find the values, we default use the previous one
	// to run properly
	LOGINFO("MapSMtoCores for SM %d.%d is undefined. Default to use %d Cores/SM\n",
					major, minor, nGpuArchCoresPerSM[index - 1].Cores);
	return nGpuArchCoresPerSM[index - 1].Cores;
}

inline const char *_ConvertSMVer2ArchName(int major, int minor) {
	// Defines for GPU Architecture types (using the SM version to determine
	// the GPU Arch name)
	typedef struct {
		int SM;  // 0xMm (hexidecimal notation), M = SM Major version,
		// and m = SM minor version
		const char *name;
	} sSMtoArchName;

	sSMtoArchName nGpuArchNameSM[] = {
			{0x30, "Kepler"},
			{0x32, "Kepler"},
			{0x35, "Kepler"},
			{0x37, "Kepler"},
			{0x50, "Maxwell"},
			{0x52, "Maxwell"},
			{0x53, "Maxwell"},
			{0x60, "Pascal"},
			{0x61, "Pascal"},
			{0x62, "Pascal"},
			{0x70, "Volta"},
			{0x72, "Xavier"},
			{0x75, "Turing"},
			{-1, "Graphics Device"}};

	int index = 0;

	while (nGpuArchNameSM[index].SM != -1) {
		if (nGpuArchNameSM[index].SM == ((major << 4) + minor)) {
			return nGpuArchNameSM[index].name;
		}

		index++;
	}

	// If we don't find the values, we default use the previous one
	// to run properly
	LOGINFO("MapSMtoArchName for SM %d.%d is undefined. Default to use %s\n",
					major,
					minor,
					nGpuArchNameSM[index - 1].name);
	return nGpuArchNameSM[index - 1].name;
}

template<typename T>
void check(T result, char const *const func, const char *const file,
					 int const line) {
	if (result) {
		LOGERROR("CUDA error at %s:%d code=%d(%s) \"%s\" \n", file, line,
						 static_cast<unsigned int>(result), cudaGetErrorName(result), func);
	}
}

// This will output the proper CUDA error strings in the event
// that a CUDA host call returns an error
#define checkCudaErrors(val) check((val), #val, __FILE__, __LINE__)

struct Cuda {
	int deviceIndex;
};

Cuda *CUDACore_Create() {

	int deviceCount;
	int pickedDeviceIndex = -1;
	int pickedTotalCores = -1;
	cudaDeviceProp pickedDevice{};
	checkCudaErrors(cudaGetDeviceCount(&deviceCount));
	LOGINFO("--- CUDA Devices ---");

	for (int i = 0u; i < deviceCount; ++i) {
		cudaDeviceProp deviceProp;
		int computeMode = -1;
		checkCudaErrors(cudaDeviceGetAttribute(&computeMode, cudaDevAttrComputeMode, i));

		if(computeMode == cudaComputeModeProhibited) continue;

		checkCudaErrors(cudaGetDeviceProperties(&deviceProp, i));

		int const coresPerSM = _ConvertSMVer2Cores(deviceProp.major, deviceProp.minor);
		int const totalCores = coresPerSM * deviceProp.multiProcessorCount;
		int const computePerf = totalCores * (deviceProp.clockRate/1024);

		LOGINFO("%d: %s %s (%d.%d)", i,
				deviceProp.name, _ConvertSMVer2ArchName(deviceProp.major, deviceProp.minor), deviceProp.major, deviceProp.minor);
		LOGINFO("%d: SMs %d, Cores %d, Total Cores %d Clock %d ~GFLOPs %f", i,
				deviceProp.multiProcessorCount, coresPerSM, totalCores, deviceProp.clockRate/1024, ((float)2 * computePerf)/1024.0f);

		// for now just pick the biggest new enough device
		if (totalCores > pickedTotalCores) {
			memcpy(&pickedDevice, &deviceProp, sizeof(cudaDeviceProp));
			pickedDeviceIndex = i;
			pickedTotalCores = totalCores;
		}
	}

	LOGINFO("---");

	if (pickedDeviceIndex == -1) {
		return nullptr;
	}

	checkCudaErrors(cudaSetDevice(pickedDeviceIndex));

	Cuda* cuda = (Cuda*)MEMORY_CALLOC(1, sizeof(Cuda));
	if(!cuda) return nullptr;

	cuda->deviceIndex = pickedDeviceIndex;
	return cuda;
}

void CUDACore_Destroy(Cuda *cuda) {
	if(!cuda) return;

	MEMORY_FREE(cuda);
}