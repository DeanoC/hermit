// License Summary: MIT see LICENSE file
#pragma once

struct Cuda;

Cuda* AccelCUDA_Create();
void AccelCUDA_Destroy(Cuda* cuda);