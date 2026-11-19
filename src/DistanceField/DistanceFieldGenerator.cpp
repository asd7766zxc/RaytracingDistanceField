#include "DistanceFieldGenerator.hpp"
#include <fstream>
#include <filesystem>


bool DistanceFieldGenerator::read_distace_field_cache(DistanceFieldData df_data, shared_ptr<Mesh> mesh){
	std::vector<float> distance_field_tmp((size_t)df_data.sx*df_data.sy*df_data.sz);
    std::string cache_path = ASSET_DIR "cache/" + std::format("{:x}",(combine_hashes(df_data.hash(),mesh->hash()))) + ".bin";
	std::ifstream cache_file(cache_path,std::ios::in | std::ios::binary);
	if (!cache_file.is_open()) {
		return false;
	}
	glBindTexture(GL_TEXTURE_3D, df_data.df_buffer.ID);
	int size = distance_field_tmp.size() * sizeof(float);
	cache_file.read(reinterpret_cast<char*>(distance_field_tmp.data()), size);

	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, df_data.sx, df_data.sy, df_data.sz, GL_RED, GL_FLOAT, distance_field_tmp.data());
	return true;
}


void DistanceFieldGenerator::write_distance_field_cache(DistanceFieldData df_data, shared_ptr<Mesh> mesh){
	std::vector<float> distance_field_tmp((size_t)df_data.sx*df_data.sy*df_data.sz);
    std::size_t size_hash = std::hash<float>{}(df_data.sizeflag);
    std::filesystem::create_directories(ASSET_DIR "cache/");
	std::string cache_path = ASSET_DIR "cache/" +  std::format("{:x}",(combine_hashes(df_data.hash(),mesh->hash()))) + ".bin";
	std::ofstream cache_file(cache_path, std::ios::out | std::ios::binary);
	if (!cache_file.is_open()) {
		return;
	}
	glBindTexture(GL_TEXTURE_3D, df_data.df_buffer.ID);
	glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, distance_field_tmp.data());
	int size = distance_field_tmp.size() * sizeof(float);
	cache_file.write(reinterpret_cast<char*>(distance_field_tmp.data()), size);
}