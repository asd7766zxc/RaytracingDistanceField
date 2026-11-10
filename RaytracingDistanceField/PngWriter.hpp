#pragma once
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace png_writer {

inline uint32_t crc32(const uint8_t* data, size_t len, uint32_t crc = 0xFFFFFFFFu) {
	static uint32_t table[256];
	static bool ready = false;
	if (!ready) {
		for (uint32_t n = 0; n < 256; ++n) {
			uint32_t c = n;
			for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			table[n] = c;
		}
		ready = true;
	}
	for (size_t i = 0; i < len; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	return crc;
}

inline void putU32(std::vector<uint8_t>& out, uint32_t v) {
	out.push_back((v >> 24) & 0xFF);
	out.push_back((v >> 16) & 0xFF);
	out.push_back((v >> 8) & 0xFF);
	out.push_back(v & 0xFF);
}

inline void putChunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
	putU32(out, (uint32_t)data.size());
	size_t start = out.size();
	out.insert(out.end(), type, type + 4);
	out.insert(out.end(), data.begin(), data.end());
	putU32(out, crc32(out.data() + start, out.size() - start) ^ 0xFFFFFFFFu);
}

inline bool writeRGB(const std::string& path, int width, int height, const uint8_t* rgb) {
	std::vector<uint8_t> raw;
	raw.reserve((size_t)(width * 3 + 1) * height);
	for (int y = 0; y < height; ++y) {
		raw.push_back(0);
		raw.insert(raw.end(), rgb + (size_t)y * width * 3, rgb + (size_t)(y + 1) * width * 3);
	}

	std::vector<uint8_t> z = { 0x78, 0x01 };
	size_t pos = 0;
	do {
		size_t len = std::min<size_t>(65535, raw.size() - pos);
		bool last = pos + len == raw.size();
		z.push_back(last ? 1 : 0);
		z.push_back(len & 0xFF);
		z.push_back((len >> 8) & 0xFF);
		z.push_back(~len & 0xFF);
		z.push_back((~len >> 8) & 0xFF);
		z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + len);
		pos += len;
	} while (pos < raw.size());
	uint32_t a = 1, b = 0;
	for (uint8_t v : raw) {
		a = (a + v) % 65521;
		b = (b + a) % 65521;
	}
	putU32(z, (b << 16) | a);

	std::vector<uint8_t> ihdr;
	putU32(ihdr, (uint32_t)width);
	putU32(ihdr, (uint32_t)height);
	ihdr.insert(ihdr.end(), { 8, 2, 0, 0, 0 });

	std::vector<uint8_t> out = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	putChunk(out, "IHDR", ihdr);
	putChunk(out, "IDAT", z);
	putChunk(out, "IEND", {});

	std::ofstream f(path, std::ios::binary);
	if (!f) return false;
	f.write((const char*)out.data(), out.size());
	return (bool)f;
}

}
