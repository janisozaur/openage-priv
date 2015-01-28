#ifndef OPENAGE_ASSETLOADER_H
#define OPENAGE_ASSETLOADER_H

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "util/dir.h"

namespace openage {

class File;

class AssetLoader
{
public:
	AssetLoader();
	AssetLoader(const std::vector<util::Dir> &paths);
	~AssetLoader();
	enum class FileRetainPolicy
	{
		UNLOAD_IMMEDIATE,
		UNLOAD_DEFERRED
	};

	void add_data_paths(const std::vector<util::Dir> &paths);
	ssize_t file_size(const std::string &filename);
	ssize_t file_size(const std::string &filename, util::Dir path);
	size_t read_whole_file(char **result, const std::string &filename);
	std::vector<std::string> file_get_lines(const std::string &file_name);
	util::Dir get_file_path(const std::string &filename) const;
	void load_file(const std::string &filename, std::function<void (const void*, const size_t)> load_func, FileRetainPolicy frp = FileRetainPolicy::UNLOAD_IMMEDIATE);

private:
	std::vector<util::Dir> dirs;
	std::map<std::string, std::shared_ptr<File>> files;
};
}


#endif // OPENAGE_ASSETLOADER_H

