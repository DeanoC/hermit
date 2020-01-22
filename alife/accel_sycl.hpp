//
// Created by Computer on 22/01/2020.
//

// License Summary: MIT see LICENSE file
#pragma once

struct Sycl;

Sycl* AccelSycl_Create();
void AccelSycl_Destroy(Sycl* sycl);