//
// Created by Computer on 21/01/2020.
//

// License Summary: MIT see LICENSE file

#include "al2o3_platform/platform.h"
#include <CL/sycl.hpp>

using namespace cl::sycl;

class example_kernel;

/* Classes can inherit from the device_selector class to allow users
 * to dictate the criteria for choosing a device from those that might be
 * present on a system. This example looks for a device with SPIR support
 * and prefers GPUs over CPUs. */
class custom_selector : public device_selector {
public:
	custom_selector() : device_selector() {}

	/* The selection is performed via the () operator in the base
	 * selector class.This method will be called once per device in each
	 * platform. Note that all platforms are evaluated whenever there is
	 * a device selection. */
	int operator()(const device &device) const override {
		LOGINFO("-----");
		LOGINFO("%s - %s", device.get_info<info::device::vendor>().c_str(),
					 device.get_info<info::device::name>().c_str());
		bool const isGPU = device.get_info<info::device::device_type>() == info::device_type::gpu;
		LOGINFO("CUs - %d, gpu - %d",
					 device.get_info<info::device::max_compute_units>(),
					 isGPU);
		return device.get_info<info::device::max_compute_units>() + (isGPU * 1024);
	}

};

int testmain() {
	const int dataSize = 2048;
	int ret = -1;
	float data[dataSize] = {0.f};

	range<1> dataRange(dataSize);
	buffer<float, 1> buf(data, dataRange);

	/* We create an object of custom_selector type and use it
	 * like any other selector. */
	custom_selector selector;
	device dev = selector.select_device();
	LOGINFO("-----\nSelected device has %d CUs @ %dMhz with %dKB private memory",
			dev.get_info<info::device::max_compute_units>(),
			    dev.get_info<info::device::max_clock_frequency>(),

				 (int)dev.get_info<info::device::local_mem_size>()/1024);
	queue myQueue(dev);

	myQueue.submit([&](handler &cgh) {
		auto ptr = buf.get_access<access::mode::read_write>(cgh);

		cgh.parallel_for<example_kernel>(dataRange, [=](item<1> item) {
			size_t idx = item.get_linear_id();
			ptr[item.get_linear_id()] = static_cast<float>(idx);
		});
	});

	/* A host accessor can be used to force an update from the device to the
	 * host, allowing the data to be checked. */
	accessor<float, 1, access::mode::read_write, access::target::host_buffer>
			hostPtr(buf);

	if (hostPtr[10] == 10.0f) {
		ret = 0;
		LOGINFO("Success");
	}

	return ret;
}
