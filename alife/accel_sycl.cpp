//
// Created by Computer on 21/01/2020.
//

// License Summary: MIT see LICENSE file

#include "al2o3_platform/platform.h"
#include "accel_sycl.hpp"
#include <CL/sycl.hpp>

struct Sycl {
	Sycl(cl::sycl::device &dev) :
			device(dev),
			queue(dev) {
	}

	cl::sycl::device device;
	cl::sycl::queue queue;
};

class selftestTag;

/* Classes can inherit from the device_selector class to allow users
 * to dictate the criteria for choosing a device from those that might be
 * present on a system. This example looks for a device with SPIR support
 * and prefers GPUs over CPUs. */
class custom_selector : public cl::sycl::device_selector {
public:
	custom_selector() : device_selector() {}

	/* The selection is performed via the () operator in the base
	 * selector class.This method will be called once per device in each
	 * platform. Note that all platforms are evaluated whenever there is
	 * a device selection. */
	int operator()(const cl::sycl::device &device) const override {
		using namespace cl::sycl;
		bool const isGPU = device.get_info<info::device::device_type>() == info::device_type::gpu;
		/*
		LOGINFO("-----");
		LOGINFO("%s - %s", device.get_info<info::device::vendor>().c_str(),
						device.get_info<info::device::name>().c_str());
		LOGINFO("CUs - %d, gpu - %d",
					 device.get_info<info::device::max_compute_units>(),
					 isGPU);*/
		return device.get_info<info::device::max_compute_units>() + (isGPU * 1024);
	}

};

Sycl *AccelSycl_Create() {
	using namespace cl::sycl;

	custom_selector selector;
	device dev = selector.select_device();
	LOGINFO("-----\nSelected device %s %s has %d CUs @ %dMhz with %dKB local memory",
					dev.get_info<info::device::vendor>().c_str(),
					dev.get_info<info::device::name>().c_str(),
					dev.get_info<info::device::max_compute_units>(),
					dev.get_info<info::device::max_clock_frequency>(),

					(int) dev.get_info<info::device::local_mem_size>() / 1024);

	auto sycl = new Sycl{dev};

	const int dataSize = 2048;
	float data[dataSize] = {0.f};

	range<1> dataRange(dataSize);
	buffer<float, 1> buf(data, dataRange);

	sycl->queue.submit([&](handler &cgh) {
		auto ptr = buf.get_access<access::mode::read_write>(cgh);

		cgh.parallel_for<selftestTag>(dataRange, [=](item<1> item) {
			size_t idx = item.get_linear_id();
			ptr[item.get_linear_id()] = static_cast<float>(idx);
		});
	});

	/* A host accessor can be used to force an update from the device to the
	 * host, allowing the data to be checked. */
	accessor<float, 1, access::mode::read_write, access::target::host_buffer>
			hostPtr(buf);

	if (hostPtr[2013] != 2013.0f) {
		LOGINFO("Sycl self test Failed");
		AccelSycl_Destroy(sycl);
		return nullptr;
	}

	return sycl;
}

void AccelSycl_Destroy(Sycl *sycl) {
	delete sycl;
}