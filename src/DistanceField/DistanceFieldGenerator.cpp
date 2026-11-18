#include "DistanceFieldGenerator.hpp"
#include <fstream>
#include <filesystem>

BYTE distance_field_tmp[256 * 256 * 256 * 4];

bool DistanceFieldGenerator::read_distace_field_cache(DistanceFieldData df_data, shared_ptr<Mesh> mesh){
    std::string cache_path = ASSET_DIR "cache/" + std::format("{:x}",(combine_hashes(df_data.hash(),mesh->hash()))) + ".bin";
	std::ifstream cache_file(cache_path,std::ios::in | std::ios::binary);
	if (!cache_file.is_open()) {
		return false;
	}
	glBindTexture(GL_TEXTURE_3D, df_data.df_buffer.ID);
	int size = sizeof(distance_field_tmp);
	cache_file.read(reinterpret_cast<char*>(distance_field_tmp), size);

	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, df_data.sx, df_data.sy, df_data.sz, GL_RED, GL_FLOAT, distance_field_tmp);
	return true;
}


void DistanceFieldGenerator::write_distance_field_cache(DistanceFieldData df_data, shared_ptr<Mesh> mesh){
    std::size_t size_hash = std::hash<float>{}(df_data.sizeflag);
    std::filesystem::create_directories(ASSET_DIR "cache/");
	std::string cache_path = ASSET_DIR "cache/" +  std::format("{:x}",(combine_hashes(df_data.hash(),mesh->hash()))) + ".bin";
	std::ofstream cache_file(cache_path, std::ios::out | std::ios::binary);
	if (!cache_file.is_open()) {
		return;
	}
	glBindTexture(GL_TEXTURE_3D, df_data.df_buffer.ID);
	glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, distance_field_tmp);
	cache_file.write(reinterpret_cast<char*>(distance_field_tmp), sizeof(distance_field_tmp));
}