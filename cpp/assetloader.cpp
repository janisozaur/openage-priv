#include "assetloader.h"
#include "util/error.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h> // for open()
#include <unistd.h> // for close(). duh.
#include <cstring>

namespace openage {

AssetLoader::AssetLoader()
{
}

AssetLoader::AssetLoader(const std::vector<util::Dir> &paths) :
	AssetLoader()
{
	this->add_data_paths(paths);
}

AssetLoader::~AssetLoader()
{
}

util::Dir AssetLoader::get_file_path(const std::string &filename) const
{
	struct stat st;
	for (auto it = this->dirs.rbegin(); it != this->dirs.rend(); it++)
	{
		if (stat((*it).join(filename).c_str(), &st) >= 0) {
			return *it;
		}
	}
	throw util::Error("File %s does not exist", filename.c_str());
}

/*
 * TODO: This function could use more regular read() calls, but hey, no buffer allocations with mmap!
 */
void AssetLoader::load_file(const std::string &filename, std::function<void (const void*, const size_t)> load_func, AssetLoader::FileRetainPolicy frp)
{
	ssize_t ssize = file_size(filename);
	size_t size;
	if (ssize < 0)
	{
		throw util::Error("failed to get file size");
	} else {
		size = (size_t)ssize;
	}
	int fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0)
	{
		throw util::Error("Error opening file %s", filename.c_str());
	}
	void *addr = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
	{
		throw util::Error("Error mmapping file %d", fd);
	}
	load_func(addr, size);
	if (frp == AssetLoader::FileRetainPolicy::UNLOAD_IMMEDIATE)
	{
		int status;
		status = munmap(addr, size);
		if (status < 0)
		{
			throw util::Error("Error munmaping addr %p (%s)", addr, filename.c_str());
		}
		status = close(fd);
		if (status < 0)
		{
			throw util::Error("Error closing fd %d (%s)", fd, filename.c_str());
		}
	} else {
		throw util::Error("Policy not implemented yet");
	}
}

void AssetLoader::add_data_paths(const std::vector<util::Dir> &paths)
{
	this->dirs.insert(this->dirs.end(), paths.cbegin(), paths.cend());
}

ssize_t AssetLoader::file_size(const std::string &filename)
{
	util::Dir dir = get_file_path(filename);
	return file_size(filename, dir);
}

ssize_t AssetLoader::file_size(const std::string &filename, util::Dir path)
{
	struct stat st;

	if (stat(path.join(filename).c_str(), &st) >= 0) {
		return st.st_size;
	}
	return -1;
}

/**
 * @brief AssetLoader::read_whole_file
 * @param[out] result   Where to allocate buffer and store content.
 *                      This function will allocate (using new[]) a buffer large enough
 *                      to store all the data to be read from requested file and 1 trailing
 *                      null byte.
 * @param[in]  filename The filename to be loaded.
 * @return This function returns size of loaded content. This function will either succeed
 *         or throw.
 */
size_t AssetLoader::read_whole_file(char **result, const std::string &filename)
{
	ssize_t content_length;

	/*
	 * This here is a lambda expression which is called with a pointer to data and its size.
	 * It's responsible for doing with that memory region whatever it needs to make the resource
	 * loaded.
	 */
	auto loader = [&result, &content_length](const void *data, const size_t size) {
		//allocate filesize + nullbyte
		*result = new char[size + 1];
		memcpy(*result, data, size);

		//make sure 0-byte is at the end
		(*result)[size] = '\0';
	};

	/*
	 * Load the file and unload it (if possible) immediatelly after loader is finished.
	 */
	load_file(filename, loader);

	// return the file size
	return content_length;
}

std::vector<std::string> AssetLoader::file_get_lines(const std::string &file_name)
{
	char *file_content;
	ssize_t fsize = read_whole_file(&file_content, file_name);

	char *file_seeker = file_content;
	char *current_line = file_content;

	auto result = std::vector<std::string>{};

	while ((size_t)file_seeker <= ((size_t)file_content + fsize)
		   && *file_seeker != '\0') {

		if (*file_seeker == '\n') {
			*file_seeker = '\0';

			result.push_back(std::string{current_line});

			current_line = file_seeker + 1;
		}
		file_seeker += 1;
	}

	delete[] file_content;
	return result;
}
}
