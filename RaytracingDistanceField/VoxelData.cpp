#include "VoxelData.hpp"
static BYTE zeros[512 * 512 * 512];
void VoxelData::resetVoxelCells() {
	voxel_buffer.BufferData(zeros);
}