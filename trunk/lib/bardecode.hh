
#include <string>
#include <vector>

std::vector<std::string> decodeBarcodes (Image& image, const std::string& codes,
					 unsigned int min_length,
                                         unsigned int max_length, int multiple);
